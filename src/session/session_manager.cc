#include "config.h"

#include "session/session_manager.h"

#include <cassert>
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
SessionManager::set_path(const std::string& path) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_freeze_info)
    throw torrent::input_error("Session path cannot be changed after startup.");

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

// TODO: Add separate mutex for queuing save requests in session save.
// TODO: Only need to save download arg, we always assume resume save is wanted.
// TODO: Use callback to main thread, generic so remove_download() doesn't get affectred.

void
SessionManager::save_download(core::Download* download, bool skip_static) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  DownloadStorer storer(download);

  storer.build_streams(skip_static);

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

    // TODO: Saving requests for full torrent should never be replaced.
    //
    // TODO: Handle remove differenty here and with remove_download(), use a bool.
    //
    // TODO: When we have the initial partial save queue, do not remove save requests from m_save_requests.

    // Rather, we should chck torrent_stream, if present we update the other streams. Remember to sanity check the path.

    if (remove_or_replace_unsafe(save_request)) {
      LT_LOG("updated pending save request : download:%p", download);
    } else {
      LT_LOG("queued save request : download:%p", download);
      m_save_requests.push_back(std::move(save_request));
    }
  }

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
}

void
SessionManager::process_save_request() {
  assert(m_thread == torrent::this_thread::thread());

  if (m_path.empty())
    return;

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::process_save_request() called while not active.");

  if (m_save_requests.empty() || m_processing_saves.size() >= max_concurrent_saves)
    return;

  process_next_save_request_unsafe();

  if (!m_save_requests.empty())
    session_thread::callback(this, [this]() { process_save_request(); });
}

void
SessionManager::process_next_save_request_unsafe() {
  auto request = std::move(m_save_requests.front());
  m_save_requests.pop_front();

  auto itr = m_processing_saves.insert(m_processing_saves.end(), ProcessingSave{});

  itr->second = std::move(request);
  itr->first = std::async(std::launch::async, [this, itr]() {
      // TODO: Properly handle errors here, and report back to session thread.
      // TODO: Consider adding a failed_saves with error info.

      DownloadStorer::save_and_move_streams(itr->second.path, m_use_fsyncdisk,
                                            *itr->second.torrent_stream,
                                            *itr->second.rtorrent_stream,
                                            *itr->second.libtorrent_stream);

      {
        std::unique_lock<std::mutex> lock(m_mutex);

        if (m_finished_saves.empty())
          session_thread::callback(this, [this]() { process_finished_saves(); });

        m_finished_saves.push_back(std::move(*itr));
        m_processing_saves.erase(itr);

        m_finished_condition.notify_all();
      }
    });
}

void
SessionManager::process_finished_saves() {
  assert(m_thread == torrent::this_thread::thread());

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::process_finished_saves() called while not active.");

  for (auto& request : m_finished_saves)
    LT_LOG("finished saving download : download:%p path:%s", request.second.download, request.second.path.c_str());

  m_finished_saves.clear();
}

void
SessionManager::wait_for_one_save_unsafe(std::unique_lock<std::mutex>& lock) {
  assert(m_thread == torrent::this_thread::thread());

  if (m_processing_saves.empty())
    return;

  m_finished_condition.wait(lock);
}

void
SessionManager::flush_all_and_wait_unsafe(std::unique_lock<std::mutex>& lock) {
  LT_LOG("flushing all pending saves", 0);

  while (!m_save_requests.empty()) {
    if (m_processing_saves.size() >= max_cleanup_saves) {
      wait_for_one_save_unsafe(lock);
      continue;
    }

    process_next_save_request_unsafe();
  }

  while (!m_processing_saves.empty())
    wait_for_one_save_unsafe(lock);

  for (auto& request : m_finished_saves)
    LT_LOG("finished saving download : download:%p path:%s", request.second.download, request.second.path.c_str());

  m_finished_saves.clear();

  LT_LOG("flushed all pending saves", 0);
}

bool
SessionManager::remove_or_replace_unsafe(SaveRequest& save_request) {
  // Can be run in any thread.

  auto itr = std::remove_if(m_save_requests.begin(), m_save_requests.end(), [download = save_request.download](auto& req) {
      return req.download == download;
    });

  if (itr == m_save_requests.end())
    return false;

  if (itr->path != save_request.path)
    throw torrent::internal_error("SessionManager::remove_or_replace_unsafe() path mismatch on replace.");

  // If this is a full save, we replace just the resume data.
  itr->rtorrent_stream   = std::move(save_request.rtorrent_stream);
  itr->libtorrent_stream = std::move(save_request.libtorrent_stream);

  // Checking active after remove_save_request_unsafe to ensure we're calling this after shutdown.
  if (!m_active)
    throw torrent::internal_error("SessionManager::remove_or_replace_unsafe() called while not active.");

  return true;
}

bool
SessionManager::remove_completely_unsafe(core::Download* download, std::unique_lock<std::mutex>& lock) {
  // Can be run in any thread.

  {
    auto itr = std::remove_if(m_save_requests.begin(), m_save_requests.end(), [download](auto& req) {
        return req.download == download;
      });

    if (itr == m_save_requests.end())
      return false;

    m_save_requests.erase(itr, m_save_requests.end());
  }

  while (true) {
    auto itr = std::find_if(m_processing_saves.begin(), m_processing_saves.end(), [download](auto& req) {
        return req.second.download == download;
      });

    if (itr == m_processing_saves.end())
      break;

    wait_for_one_save_unsafe(lock);
  }

  // Checking active after remove_save_request_unsafe to ensure we're calling this after shutdown.
  if (!m_active)
    throw torrent::internal_error("SessionManager::remove_completely_unsafe() called while not active.");

  return true;
}

// TODO: Properly handle errors.
// TODO: If no more sockets can be opened, wait for a job to finish. if all is finished, use a
// timeout nad try again.

}  // namespace session

