#include "config.h"

#include "session_manager.h"

#include <cassert>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <torrent/utils/log.h>

#include "globals.h"

#define LT_LOG(log_fmt, ...)                                            \
  lt_log_print(torrent::LOG_SESSION_EVENTS, "session-events: " log_fmt, __VA_ARGS__);

namespace session {

// TODO: Add session save scheduler that runs in main thread and passes download one-by-one to session manager.

SessionManager::SessionManager(torrent::utils::Thread* thread)
  : m_thread(thread) {
}

SessionManager::~SessionManager() = default;

// TODO:
//  * Lock session directory.
//  * On shutdown, wait for all saves to finish.
//    * Add is_empty_and_done() that also include async fdisksync tasks.
//  * Then unlock session directory.

bool
SessionManager::is_empty() {
  std::lock_guard<std::mutex> guard(m_mutex);
  return m_save_requests.empty();
}

// TODO: Derive path from download info hash.
// TODO: Generate streams here, not in download store.
void
SessionManager::save_download(core::Download* download, std::string path, stream_ptr torrent_stream, stream_ptr rtorrent_stream, stream_ptr libtorrent_stream) {
  assert(m_thread != torrent::this_thread::thread());

  {
    std::lock_guard<std::mutex> guard(m_mutex);

    LT_LOG("requesting save : download:%p path:%s", download, path.c_str());

    // TODO: Remove is already queued entries.

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
SessionManager::cancel_download(core::Download* download) {
  assert(m_thread != torrent::this_thread::thread());

  std::lock_guard<std::mutex> guard(m_mutex);

  // TODO: Add these to a temp structure, and remove from to-be-added temp struct.

  auto itr = std::remove_if(m_save_requests.begin(), m_save_requests.end(), [download](auto& req) {
      return req.download == download;
    });

  if (itr == m_save_requests.end()) {
    LT_LOG("skipping cancel save request : download:%p", download);
    return;
  }

  LT_LOG("canceling save request : download:%p", download);

  m_save_requests.erase(itr, m_save_requests.end());

  // TODO: Use atomic download ptr to check if we're currently processing this download
  // TODO: If so, use a lock to wait for save to finish before returning
}

void
SessionManager::process_save_request() {
  assert(m_thread == torrent::this_thread::thread());

  std::lock_guard<std::mutex> guard(m_mutex);

  // pick first request
  // process it
  // add us back to callbacks? we need to disable shutdown while processing all saves... do we do it at thread cleanup?

  auto request = std::move(m_save_requests.front());
  m_save_requests.pop_front();

  // Keep lock while processing to ensure cancellations do not interfere.

  save_download_unsafe(request);

  if (!m_save_requests.empty())
    session_thread::callback(nullptr, [this]() { process_save_request(); });
}

// TODO: Add threads/tasklets that calls fdisksync on shutdown.
// TODO: Parallelize saves.

// TODO: Properly handle errors.
void
SessionManager::save_download_unsafe(const SaveRequest& request) {
  LT_LOG("saving download : download:%p path:%s", request.download, request.path.c_str());

  auto torrent_path    = request.path;
  auto libtorrent_path = request.path + ".libtorrent_resume";
  auto rtorrent_path   = request.path + ".rtorrent";

  if (request.torrent_stream) {
    if (!save_download_stream_unsafe(torrent_path, request.torrent_stream))
      return;
  }

  if (!save_download_stream_unsafe(libtorrent_path, request.libtorrent_stream))
    return;

  if (!save_download_stream_unsafe(rtorrent_path, request.rtorrent_stream))
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

  // if (rpc::call_command_value("system.files.session.fdatasync")) {
  if (true) {
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

