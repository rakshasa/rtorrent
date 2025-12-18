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
// TODO: Generate streams here, not in download store.
void
SessionManager::save_download(core::Download* download, std::string path, stream_ptr torrent_stream, stream_ptr rtorrent_stream, stream_ptr libtorrent_stream) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  {
    std::lock_guard<std::mutex> guard(m_mutex);

    LT_LOG("requesting save : download:%p path:%s", download, path.c_str());

    if (!m_active)
      throw torrent::internal_error("SessionManager::save_download() called while not active.");

    if (remove_save_request_unsafe(download))
      LT_LOG("replacing pending save request : download:%p", download);

    // TODO: Add these to a temp structure
    // TODO: When a download already exists, replace it.

    m_save_requests.push_back(SaveRequest{
        download,
        std::move(path),
        std::move(torrent_stream),
        std::move(rtorrent_stream),
        std::move(libtorrent_stream)
      });
  }

  session_thread::callback(nullptr, [this]() { process_save_request(); });
}

void
SessionManager::remove_download(core::Download* download, std::string base_path) {
  assert(torrent::this_thread::thread() == torrent::main_thread::thread());

  if (m_path.empty())
    return;

  std::lock_guard<std::mutex> guard(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::remove_download() called while not active.");

  // TODO: Add these to a temp structure, and remove from to-be-added temp struct.

  if (remove_save_request_unsafe(download))
    LT_LOG("canceled pending save request : download:%p", download);

  // TODO: Use atomic download ptr to check if we're currently processing this download
  // TODO: If so, use a lock to wait for save to finish before returning

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

  std::lock_guard<std::mutex> guard(m_mutex);

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

  std::lock_guard<std::mutex> guard(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::cleanup() called while not active.");

  m_active = false;

  if (m_path.empty()) {
    LT_LOG("session manager cleanup called with empty path, skipping", 0);
    return;
  }

  LT_LOG("flushing pending saves before cleanup", 0);

  while (!m_save_requests.empty()) {
    auto request = std::move(m_save_requests.front());
    m_save_requests.pop_front();

    save_download_unsafe(request);
  }

  // TODO: Wait for async fdisksync tasks to finish.

  LT_LOG("cleaning up session manager with path: %s", m_path.c_str());

  if (m_use_lock) {
    if (!m_lockfile->unlock())
      LT_LOG("could not unlock session directory: %s", m_path.c_str());

    LT_LOG("unlocked session directory: %s", m_path.c_str());
  }
}

void
SessionManager::process_save_request() {
  assert(m_thread == torrent::this_thread::thread());

  if (m_path.empty())
    return;

  std::lock_guard<std::mutex> guard(m_mutex);

  if (!m_active)
    throw torrent::internal_error("SessionManager::process_save_request() called while not active.");

  if (m_save_requests.empty())
    return;

  auto request = std::move(m_save_requests.front());
  m_save_requests.pop_front();

  // Keep lock while processing to ensure cancellations do not interfere.

  save_download_unsafe(request);

  if (!m_save_requests.empty())
    session_thread::callback(nullptr, [this]() { process_save_request(); });
}

bool
SessionManager::remove_save_request_unsafe(core::Download* download) {
  assert(m_thread == torrent::this_thread::thread());

  auto itr = std::remove_if(m_save_requests.begin(), m_save_requests.end(), [download](auto& req) {
      return req.download == download;
    });

  if (itr == m_save_requests.end())
    return false;

  m_save_requests.erase(itr, m_save_requests.end());
  return true;
}

// TODO: Add threads/tasklets that calls fdisksync on shutdown.
// TODO: Parallelize saves.
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

  // We don't care about cancelation here, as the underlying file gets deleted / replaced anyway.

  // TODO: We can use std::async for these, and only wait for them if we're shutting down.

  // TODO: Use an atomic counter / conditional variable to keep track of async operations count, and
  // wait for finished operations on shutdown.

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

