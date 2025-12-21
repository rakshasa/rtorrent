#ifndef RTORRENT_SESSION_DOWNLOAD_STORER_H
#define RTORRENT_SESSION_DOWNLOAD_STORER_H

#include <memory>
#include <sstream>
#include <string>
#include <torrent/common.h>

namespace core {
class Download;
}

namespace utils {
  class Directory;
}

namespace session {

class DownloadStorer {
public:
  DownloadStorer(core::Download* download);

  core::Download*     download() const { return m_download; }

  void                build_streams(bool skip_static);
  std::string         build_path(const std::string& session_path);

  auto                torrent_stream()    { return std::move(m_torrent_stream); }
  auto                rtorrent_stream()   { return std::move(m_rtorrent_stream); }
  auto                libtorrent_stream() { return std::move(m_libtorrent_stream); }

  static utils::Directory get_formated_entries(const std::string& session_path);

private:
  core::Download*     m_download;

  std::unique_ptr<std::stringstream> m_torrent_stream;
  std::unique_ptr<std::stringstream> m_rtorrent_stream;
  std::unique_ptr<std::stringstream> m_libtorrent_stream;
};

} // namespace session

#endif // RTORRENT_SESSION_DOWNLOAD_STORER_H
