#include "config.h"

#include "session_manager.h"

#include <cassert>

// TODO: Add logging.

namespace session {

// TODO: Lock session directory.
// TODO: Add session save scheduler that runs in main thread and passes download one-by-one to session manager.

// TODO: Should we rename this to SessionSaver, or RequestQueue?

SessionManager::SessionManager(torrent::utils::Thread* thread)
  : m_thread(thread) {
}

SessionManager::~SessionManager() = default;

// TODO: Derive path from download info hash.
// TODO: Generate streams here, not in download store.
void
SessionManager::save_download(core::Download* download, std::string path, stream_ptr download_stream, stream_ptr rtorrent_stream, stream_ptr libtorrent_stream) {
  assert(m_thread != torrent::this_thread::thread());

  std::lock_guard<std::mutex> guard(m_mutex);

  // TODO: Add these to a temp structure
  // TODO: When a download already exists, replace it.

  m_save_requests.push_back(SaveRequest{
      download,
      std::move(path),
      std::move(download_stream),
      std::move(rtorrent_stream),
      std::move(libtorrent_stream)
    });

  // TODO: Poke the session thread to process save requests.
}

void
SessionManager::cancel_download(core::Download* download) {
  assert(m_thread != torrent::this_thread::thread());

  std::lock_guard<std::mutex> guard(m_mutex);

  // TODO: Add these to a temp structure, and remove from to-be-added temp struct.

  m_save_requests.erase(std::remove_if(m_save_requests.begin(), m_save_requests.end(),
                                       [download](auto& req) { return req.download == download; }),
                        m_save_requests.end());

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
}

void
SessionManager::save_download_unsafe(const SaveRequest& request) {
}

}  // namespace session

