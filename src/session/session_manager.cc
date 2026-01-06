#include "config.h"

#include "session/session_manager.h"

#include <cassert>
#include <cstdlib>
#include <torrent/exceptions.h>
#include <torrent/utils/log.h>

#include "globals.h"
#include "session/download_storer.h"
#include "utils/lockfile.h"

#define LT_LOG(log_fmt, ...)                                            \
  lt_log_print(torrent::LOG_SESSION_EVENTS, "session-events: " log_fmt, __VA_ARGS__);

namespace session {

SessionManager::SessionManager(torrent::utils::Thread* thread)
  : m_thread(thread),
    m_lockfile(std::make_unique<utils::Lockfile>()) {
}

SessionManager::~SessionManager() = default;

void
SessionManager::set_path(std::string path) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_freeze_info)
    throw torrent::input_error("Session path cannot be changed after startup.");

  path = expand_path(path);

  if (path.empty() || path.back() == '/')
    m_path = path;
  else
    m_path = path + '/';
}

void
SessionManager::set_use_fsyncdisk(bool use_fsyncdisk) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_freeze_info)
    throw torrent::input_error("Session fsyncdisk option cannot be changed after startup.");

  m_use_fsyncdisk = use_fsyncdisk;
}

void
SessionManager::set_use_lock(bool use_lock) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_freeze_info)
    throw torrent::input_error("Session lock option cannot be changed after startup.");

  m_use_lock = use_lock;
}

void
SessionManager::save_resume_download(core::Download* download) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  {
    std::unique_lock<std::mutex> lock(m_pending_builds_mutex);

    if (!m_active)
      throw torrent::internal_error("SessionManager::save_resume_download() called while not active.");

    if (std::find(m_pending_builds.begin(), m_pending_builds.end(), download) != m_pending_builds.end()) {
      LT_LOG("download already in pending build of resume save : download:%p", download);
      return;
    }

    if (m_pending_builds.empty())
      torrent::main_thread::callback(this, [this]() { process_pending_resume_builds(false); });

    m_pending_builds.push_back(download);

    LT_LOG("build of resume save data queued : download:%p", download);
  }
}

void
SessionManager::save_full_download(core::Download* download) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  DownloadStorer storer(download);

  storer.build_full_streams();

  auto save_request = SaveRequest{
    download,
    storer.build_path(m_path),
    storer.torrent_stream(),
    storer.rtorrent_stream(),
    storer.libtorrent_stream()
  };

  {
    std::unique_lock<std::mutex> lock(m_mutex);

    LT_LOG("requesting save : download:%p path:%s", download, save_request.path.c_str());

    if (!m_active)
      throw torrent::internal_error("SessionManager::save_download() called while not active.");

    if (replace_save_request_unsafe(save_request))
      throw torrent::internal_error("SessionManager::save_full_download() replacing existing save request, not supported?");

    m_save_requests.push_back(std::move(save_request));
    m_save_request_counter = m_save_requests.size();

    LT_LOG("queued new full save request : download:%p", download);
  }

  if (!m_processing_saves_callback_scheduled.exchange(true))
    session_thread::callback(this, [this]() { process_save_request(); });
}

void
SessionManager::remove_download(core::Download* download) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::remove_download() called while not active.");

  if (remove_completely_unsafe(download, lock))
    LT_LOG("canceled pending save request : download:%p", download);

  DownloadStorer(download).unlink_files(m_path);

  LT_LOG("removed session files : download:%p", download);
}

void
SessionManager::start() {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  std::unique_lock<std::mutex> lock(m_mutex);

  if (m_active || m_freeze_info)
    throw torrent::internal_error("SessionManager::start() called while already started.");

  m_active      = true;
  m_freeze_info = true;

  if (m_path.empty()) {
    LT_LOG("session manager started with empty path, disabling session management", 0);
    return;
  }

  LT_LOG("starting session manager with path: %s", m_path.c_str());

  if (m_use_lock) {
    m_lockfile->set_path(m_path + "rtorrent.lock");

    if (!m_lockfile->try_lock()) {
      if (errno == ENOENT || errno == ENOTDIR || errno == EACCES)
        throw torrent::input_error("Could not lock session directory: " + std::string(std::strerror(errno)) + " : " + m_path);
      else
        throw torrent::input_error("Could not lock session directory, held by: " + m_lockfile->locked_by_as_string() + " : " + m_path);
    }

    LT_LOG("locked session directory: %s", m_path.c_str());
  }
}

void
SessionManager::cleanup() {
  assert(m_thread == torrent::this_thread::thread());

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::cleanup() called while not active.");

  {
    std::unique_lock<std::mutex> pending_lock(m_pending_builds_mutex);

    if (!m_pending_builds.empty())
      throw torrent::internal_error("SessionManager::cleanup() called with pending builds.");
  }

  m_active = false;

  if (m_path.empty()) {
    LT_LOG("session manager cleanup called with empty path, skipping", 0);
    return;
  }

  LT_LOG("cleaning up session manager with path: %s", m_path.c_str());

  flush_all_and_wait_unsafe(lock);

  if (m_use_lock) {
    if (!m_lockfile->unlock())
      LT_LOG("could not unlock session directory: %s", m_path.c_str());

    LT_LOG("unlocked session directory: %s", m_path.c_str());
  }

  LT_LOG("session manager cleaned up", 0);

  session_thread::cancel_callback(this);
  torrent::main_thread::cancel_callback(this);
}

void
SessionManager::process_pending_resume_builds(bool is_flushing) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  std::vector<SaveRequest> requests;

  // Only main thread is allowed to process pending builds or remove downloads, as such it is safe
  // to unlock before adding them to save requests.
  {
    std::unique_lock<std::mutex> lock(m_pending_builds_mutex);

    while (!m_pending_builds.empty()) {
      if (!is_flushing && m_save_request_counter + requests.size() >= max_concurrent_requests)
        break;

      auto* download = m_pending_builds.front();
      m_pending_builds.pop_front();

      LT_LOG("processing pending resume save : download:%p", download);

      DownloadStorer storer(download);

      storer.build_resume_streams();

      auto save_request = SaveRequest{
        download,
        storer.build_path(m_path),
        nullptr,
        storer.rtorrent_stream(),
        storer.libtorrent_stream()
      };

      requests.push_back(std::move(save_request));
    }
  }

  if (requests.empty())
    return;

  {
    std::unique_lock<std::mutex> save_lock(m_mutex);

    if (!m_active)
      throw torrent::internal_error("SessionManager::process_pending_builds() called while not active.");

    for (auto& save_request : requests) {
      if (replace_save_request_unsafe(save_request)) {
        LT_LOG("updated pending resume save request : download:%p", save_request.download);
        continue;
      }

      m_save_requests.push_back(std::move(save_request));

      LT_LOG("queued new resume save request : download:%p", save_request.download);
    }

    m_save_request_counter = m_save_requests.size();
  }

  if (!is_flushing && !m_processing_saves_callback_scheduled.exchange(true))
    session_thread::callback(this, [this]() { process_save_request_with_pending_callback(); });
}

void
SessionManager::process_save_request() {
  assert(m_thread == torrent::this_thread::thread());

  if (m_path.empty())
    throw torrent::internal_error("SessionManager::process_save_request() called with empty path.");

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::process_save_request() called while not active.");

  while (!m_save_requests.empty() && m_processing_saves.size() < max_concurrent_requests)
    process_next_save_request_unsafe();

  m_processing_saves_callback_scheduled = false;
}

void
SessionManager::process_save_request_with_pending_callback() {
  process_save_request();

  std::unique_lock<std::mutex> lock(m_pending_builds_mutex);

  if (!m_pending_builds.empty())
    torrent::main_thread::callback(this, [this]() { process_pending_resume_builds(false); });
}

void
SessionManager::process_next_save_request_unsafe() {
  auto request = std::move(m_save_requests.front());

  m_save_requests.pop_front();
  m_save_request_counter = m_save_requests.size();

  auto itr = m_processing_saves.insert(m_processing_saves.end(), ProcessingSave{});

  itr->second = std::move(request);
  itr->first = std::async(std::launch::async, [this, itr]() {
      auto cleanup_fn = [this, itr]() {
          std::unique_lock<std::mutex> lock(m_mutex);

          if (m_finished_saves.empty())
            session_thread::callback(this, [this]() { process_finished_saves(); });

          m_finished_saves.push_back(std::move(*itr));
          m_finished_condition.notify_all();

          m_processing_saves.erase(itr);
        };

      try {
        DownloadStorer::save_and_move_streams(itr->second.path, m_use_fsyncdisk,
                                              itr->second.torrent_stream.get(),
                                              itr->second.rtorrent_stream.get(),
                                              itr->second.libtorrent_stream.get());
      } catch (...) {
        cleanup_fn();
        throw;
      }

      cleanup_fn();
    });
}

void
SessionManager::process_finished_saves() {
  assert(m_thread == torrent::this_thread::thread());

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::process_finished_saves() called while not active.");

  for (auto& request : m_finished_saves) {
    try {
      request.first.get();

    } catch (torrent::storage_error& e) {
      LT_LOG("error saving download : storage error :download:%p path:%s : %s", request.second.download, request.second.path.c_str(), e.what());

      if (m_last_storage_error_message + std::chrono::minutes(5) > torrent::this_thread::cached_time()) {
        m_ignored_storage_error_count++;
        continue;
      }

      lt_log_print(torrent::LOG_ERROR, "Storage errors saving session data for download: ignored:%u : %s", m_ignored_storage_error_count, e.what());

      m_last_storage_error_message = torrent::this_thread::cached_time();
      m_ignored_storage_error_count = 0;
      continue;

    } catch (torrent::internal_error& e) {
      LT_LOG("error saving download : internal error : download:%p path:%s : %s", request.second.download, request.second.path.c_str(), e.what());
      throw;

    } catch (...) {
      LT_LOG("error saving download : unknown error : download:%p path:%s", request.second.download, request.second.path.c_str());
      throw;
    }

    LT_LOG("finished saving download : download:%p path:%s", request.second.download, request.second.path.c_str());
  }

  m_finished_saves.clear();
}

void
SessionManager::flush_all_and_wait_unsafe(std::unique_lock<std::mutex>& lock) {
  LT_LOG("flushing all pending saves", 0);

  while (!m_save_requests.empty()) {
    if (m_processing_saves.size() >= max_cleanup_processing) {
      m_finished_condition.wait(lock);
      continue;
    }

    process_next_save_request_unsafe();
  }

  while (!m_processing_saves.empty())
    m_finished_condition.wait(lock);

  for (auto& request : m_finished_saves)
    LT_LOG("finished saving download : download:%p path:%s", request.second.download, request.second.path.c_str());

  m_finished_saves.clear();

  LT_LOG("flushed all pending saves", 0);
}

bool
SessionManager::replace_save_request_unsafe(SaveRequest& save_request) {
  // Can be run in any thread.

  auto itr = std::find_if(m_save_requests.begin(), m_save_requests.end(), [download = save_request.download](auto& req) {
      return req.download == download;
    });

  if (itr == m_save_requests.end())
    return false;

  if (itr->path != save_request.path)
    throw torrent::internal_error("SessionManager::replace_save_request_unsafe() path mismatch on replace: " + itr->path + " != " + save_request.path);

  if (save_request.torrent_stream != nullptr)
    throw torrent::internal_error("SessionManager::replace_save_request_unsafe() cannot replace full save requests.");

  itr->rtorrent_stream   = std::move(save_request.rtorrent_stream);
  itr->libtorrent_stream = std::move(save_request.libtorrent_stream);

  return true;
}

bool
SessionManager::remove_completely_unsafe(core::Download* download, std::unique_lock<std::mutex>& lock) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  auto remove_requests = [this, download]() {
    auto itr = std::remove_if(m_save_requests.begin(), m_save_requests.end(), [download](auto& req) {
        return req.download == download;
      });

    if (itr == m_save_requests.end())
      return false;

    m_save_requests.erase(itr, m_save_requests.end());
    return true;
  };

  auto remove_pending = [this, download]() {
    std::unique_lock<std::mutex> pending_lock(m_pending_builds_mutex);

    auto itr = std::remove_if(m_pending_builds.begin(), m_pending_builds.end(), [download](auto* req) {
        return req == download;
      });

    if (itr == m_pending_builds.end())
      return false;

    m_pending_builds.erase(itr, m_pending_builds.end());
    return true;
  };

  bool removed_something = false;

  if (remove_pending())
    removed_something = true;

  if (remove_requests())
    removed_something = true;

  // This may block for a relatively long time if fdatasync is in use, however this is necessary.
  while (true) {
    auto itr = std::find_if(m_processing_saves.begin(), m_processing_saves.end(), [download](auto& req) {
        return req.second.download == download;
      });

    if (itr == m_processing_saves.end())
      break;

    m_finished_condition.wait(lock);
  }

  // Since we're the main thread, no more requests for this download can be added.

  // Checking active after remove_save_request_unsafe to ensure we're calling this after shutdown.
  if (!m_active)
    throw torrent::internal_error("SessionManager::remove_completely_unsafe() called while not active.");

  return removed_something;
}

// TODO: Properly handle errors.
// TODO: If no more sockets can be opened, wait for a job to finish. if all is finished, use a
// timeout and try again.

}  // namespace session

