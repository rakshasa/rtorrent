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

  std::string         build_path(const std::string& session_path);

  void                build_full_streams()   { build_streams(false); }
  void                build_resume_streams() { build_streams(true); }

  void                unlink_files(const std::string& session_path);

  auto                torrent_stream()       { return std::move(m_torrent_stream); }
  auto                rtorrent_stream()      { return std::move(m_rtorrent_stream); }
  auto                libtorrent_stream()    { return std::move(m_libtorrent_stream); }

  static void         save_and_move_streams(const std::string& path, bool use_fsyncdisk,
                                            const std::stringstream& torrent_stream,
                                            const std::stringstream& rtorrent_stream,
                                            const std::stringstream& libtorrent_stream);

  static utils::Directory get_formated_entries(const std::string& session_path);

private:
  void                build_streams(bool skip_static);

  core::Download*     m_download;

  std::unique_ptr<std::stringstream> m_torrent_stream;
  std::unique_ptr<std::stringstream> m_rtorrent_stream;
  std::unique_ptr<std::stringstream> m_libtorrent_stream;
};

} // namespace session

#endif // RTORRENT_SESSION_DOWNLOAD_STORER_H
