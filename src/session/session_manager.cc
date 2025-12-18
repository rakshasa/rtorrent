#include "config.h"

#include "session_manager.h"

#include <cassert>
#include <cerrno>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <torrent/exceptions.h>
#include <torrent/utils/log.h>

#include "globals.h"
#include "utils/lockfile.h"

#define LT_LOG(log_fmt, ...)                                            \
  lt_log_print(torrent::LOG_SESSION_EVENTS, "session-events: " log_fmt, __VA_ARGS__);

namespace session {

// TODO: Add session save scheduler that runs in main thread and passes download one-by-one to session manager.

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

// TODO: Derive path from download info hash.
// TODO: Generate streams here, not in download store?
void
SessionManager::save_download(core::Download* download, std::string path, stream_ptr torrent_stream, stream_ptr rtorrent_stream, stream_ptr libtorrent_stream) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  {
    std::unique_lock<std::mutex> lock(m_mutex);

    LT_LOG("requesting save : download:%p path:%s", download, path.c_str());

    if (!m_active)
      throw torrent::internal_error("SessionManager::save_download() called while not active.");

    if (remove_save_request_unsafe(download, lock))
      LT_LOG("replacing pending save request : download:%p", download);

    m_save_requests.push_back(SaveRequest{
        download,
        std::move(path),
        std::move(torrent_stream),
        std::move(rtorrent_stream),
        std::move(libtorrent_stream)
      });
  }

  session_thread::callback(this, [this]() { process_save_request(); });
}

void
SessionManager::remove_download(core::Download* download, std::string base_path) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::remove_download() called while not active.");

  if (remove_save_request_unsafe(download, lock))
    LT_LOG("canceled pending save request : download:%p", download);

  auto torrent_path    = base_path;
  auto libtorrent_path = base_path + ".libtorrent_resume";
  auto rtorrent_path   = base_path + ".rtorrent";

  ::unlink(libtorrent_path.c_str());
  ::unlink(rtorrent_path.c_str());
  ::unlink(torrent_path.c_str());
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
      std::unique_lock<std::mutex> lock(m_mutex);

      // TODO: Make sure we check for exceptions on future.
      save_download_unsafe(itr->second);

      if (m_finished_saves.empty())
        session_thread::callback(this, [this]() { process_finished_saves(); });

      m_finished_saves.push_back(std::move(itr->second));
      m_processing_saves.erase(itr);
    });
}

void
SessionManager::process_finished_saves() {
  assert(m_thread == torrent::this_thread::thread());

  std::unique_lock<std::mutex> lock(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::process_finished_saves() called while not active.");

  for (auto& request : m_finished_saves)
    LT_LOG("finished saving download : download:%p path:%s", request.download, request.path.c_str());

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
    process_next_save_request_unsafe();

    if (m_processing_saves.size() >= max_concurrent_saves)
      wait_for_one_save_unsafe(lock);
  }

  while (!m_processing_saves.empty())
    wait_for_one_save_unsafe(lock);

  for (auto& request : m_finished_saves)
    LT_LOG("finished saving download : download:%p path:%s", request.download, request.path.c_str());

  m_finished_saves.clear();

  LT_LOG("flushed all pending saves", 0);
}

bool
SessionManager::remove_save_request_unsafe(core::Download* download, std::unique_lock<std::mutex>& lock) {
  // Can be run in any thread.

  {
    auto itr = std::remove_if(m_save_requests.begin(), m_save_requests.end(), [download](auto& req) {
        return req.download == download;
      });

    if (itr == m_save_requests.end())
      return false;

    m_save_requests.erase(itr, m_save_requests.end());
  }

  // check if in processing saves, then wait for it to finish using wait_for_one_save_unsafe()
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
    throw torrent::internal_error("SessionManager::remove_save_request_unsafe() called while not active");

  return true;
}

// TODO: Properly handle errors.

void
SessionManager::save_download_unsafe(const SaveRequest& request) {
  LT_LOG("saving download : download:%p path:%s", request.download, request.path.c_str());

  if (m_path.empty())
    throw torrent::internal_error("SessionManager::save_download_unsafe() called with empty session path.");

  auto torrent_path    = request.path;
  auto libtorrent_path = request.path + ".libtorrent_resume";
  auto rtorrent_path   = request.path + ".rtorrent";

  if (request.torrent_stream) {
    if (!save_download_stream_unsafe(torrent_path + ".new", request.torrent_stream))
      return;
  }

  if (!save_download_stream_unsafe(libtorrent_path + ".new", request.libtorrent_stream))
    return;

  if (!save_download_stream_unsafe(rtorrent_path + ".new", request.rtorrent_stream))
    return;

  if (request.torrent_stream) {
    if (::rename((torrent_path + ".new").c_str(), torrent_path.c_str()) == -1) {
      LT_LOG("failed to rename torrent file : %s", torrent_path.c_str());
      return;
    }
  }

  if (::rename((libtorrent_path + ".new").c_str(), libtorrent_path.c_str()) == -1) {
    LT_LOG("failed to rename libtorrent resume file : %s", libtorrent_path.c_str());
    return;
  }

  if (::rename((rtorrent_path + ".new").c_str(), rtorrent_path.c_str()) == -1) {
    LT_LOG("failed to rename rtorrent resume file : %s", rtorrent_path.c_str());
    return;
  }
}

// TODO: Rewrite to be all done in std::async, and from rdbuf directly to fd to avoid re-opening.

bool
SessionManager::save_download_stream_unsafe(const std::string& path, const std::unique_ptr<std::stringstream>& stream) {
  std::fstream output(path.c_str(), std::ios::out | std::ios::trunc);

  if (!output.is_open()) {
    LT_LOG("failed to open file for writing : path:%s", path.c_str());
    return false;
  }

  output << stream->rdbuf();

  if (!output.good()) {
    LT_LOG("failed to write stream to file : path:%s", path.c_str());
    return false;
  }

  output.close();

  // Ensure that the new file is actually written to the disk
  int fd = ::open(path.c_str(), O_WRONLY);

  if (fd < 0) {
    LT_LOG("failed to open file descriptor for fdatasync : path:%s", path.c_str());
    return false;
  }

  if (m_use_fsyncdisk) {
#ifdef __APPLE__
    ::fsync(fd);
#else
    ::fdatasync(fd);
#endif
  }

  ::close(fd);
  return true;
}

}  // namespace session

