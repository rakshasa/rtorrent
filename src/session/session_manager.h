#ifndef RTORRENT_SESSION_SESSION_MANAGER_H
#define RTORRENT_SESSION_SESSION_MANAGER_H

#include <deque>
#include <memory>
#include <sstream>
#include <string>
#include <torrent/common.h>

namespace core {

class Download;

} // namespace core

namespace session {

struct SaveRequest {
  core::Download*                    download;
  std::string                        path;
  std::unique_ptr<std::stringstream> download_stream;
  std::unique_ptr<std::stringstream> rtorrent_stream;
  std::unique_ptr<std::stringstream> libtorrent_stream;
};

class SessionManager {
public:
  typedef std::unique_ptr<std::stringstream> stream_ptr;

  SessionManager(torrent::utils::Thread* thread);
  ~SessionManager();

  void                save_download(core::Download* download, std::string path, stream_ptr download_stream, stream_ptr rtorrent_stream, stream_ptr libtorrent_stream);
  void                cancel_download(core::Download* download);

private:
  void                process_save_request();

  void                save_download_unsafe(const SaveRequest& request);

  torrent::utils::Thread* m_thread;

  std::mutex              m_mutex;
  std::deque<SaveRequest> m_save_requests;
};

} // namespace session

#endif // RTORRENT_SESSION_SESSION_MANAGER_H
