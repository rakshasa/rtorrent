#include "config.h"

#include "download_storer.h"

#include <fcntl.h>
#include <fstream>
#include <unistd.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/object_stream.h>
#include <torrent/utils/log.h>
#include <torrent/utils/resume.h>

#include "globals.h"
#include "core/download.h"
#include "utils/directory.h"

namespace session {

DownloadStorer::DownloadStorer(core::Download* download)
  : m_download(download) {
}

void
DownloadStorer::build_streams(bool skip_static) {
  auto* download = m_download->download();

  auto& resume_base   = download->bencode()->get_key("libtorrent_resume");
  auto& rtorrent_base = download->bencode()->get_key("rtorrent");

  rtorrent_base.insert_key("chunks_done",      download->file_list()->completed_chunks());
  rtorrent_base.insert_key("chunks_wanted",    download->data()->wanted_chunks());
  rtorrent_base.insert_key("total_uploaded",   m_download->info()->up_rate()->total());
  rtorrent_base.insert_key("total_downloaded", m_download->info()->down_rate()->total());

  // Don't save for completed torrents when we've cleared the uncertain_pieces.
  torrent::resume_save_progress(*download, resume_base);
  torrent::resume_save_uncertain_pieces(*download, resume_base);

  torrent::resume_save_addresses(*download, resume_base);
  torrent::resume_save_file_priorities(*download, resume_base);
  torrent::resume_save_tracker_settings(*download, resume_base);

  // Temp fixing of all flags, move to a better place:
  resume_base.set_flags(torrent::Object::flag_session_data);
  rtorrent_base.set_flags(torrent::Object::flag_session_data);

  auto torrent_stream  = std::unique_ptr<std::stringstream>();
  auto resume_stream   = std::make_unique<std::stringstream>();
  auto rtorrent_stream = std::make_unique<std::stringstream>();

  if (!skip_static) {
    torrent_stream = std::make_unique<std::stringstream>();
    torrent::object_write_bencode(&*torrent_stream, m_download->bencode(), torrent::Object::flag_session_data);

    if (!torrent_stream->good())
      throw torrent::internal_error("DownloadStorer::build_streams() failed to write torrent stream.");
  }

  torrent::object_write_bencode(&*resume_stream, &resume_base, 0);

  if (!resume_stream->good())
    throw torrent::internal_error("DownloadStorer::build_streams() failed to write resume stream.");

  torrent::object_write_bencode(&*rtorrent_stream, &rtorrent_base, 0);

  if (!rtorrent_stream->good())
    throw torrent::internal_error("DownloadStorer::build_streams() failed to write rtorrent stream.");

  m_torrent_stream    = std::move(torrent_stream);
  m_rtorrent_stream   = std::move(rtorrent_stream);
  m_libtorrent_stream = std::move(resume_stream);
}

std::string
DownloadStorer::build_path(const std::string& session_path) {
  auto info_hash = m_download->info()->info_hash();

  if (session_path.empty())
    throw torrent::internal_error("DownloadStorer::build_path() called with empty session path.");

  if (session_path.back() != '/')
    throw torrent::internal_error("DownloadStorer::build_path() session path missing trailing slash.");

  return session_path + torrent::hash_string_to_hex_str(info_hash) + ".torrent";
}

void
DownloadStorer::unlink_files(const std::string& session_path) {
  auto base_path = build_path(session_path);

  auto torrent_path    = base_path;
  auto libtorrent_path = base_path + ".libtorrent_resume";
  auto rtorrent_path   = base_path + ".rtorrent";

  ::unlink(libtorrent_path.c_str());
  ::unlink(rtorrent_path.c_str());
  ::unlink(torrent_path.c_str());
}

namespace {

bool
is_correct_format(const std::string& f) {
  if (f.size() != 48 || f.substr(40) != ".torrent")
    return false;

  for (std::string::const_iterator itr = f.begin(); itr != f.end() - 8; ++itr)
    if (!(*itr >= '0' && *itr <= '9') &&
        !(*itr >= 'A' && *itr <= 'F'))
      return false;

  return true;
}

bool
save_stream(const std::string& path, bool use_fsyncdisk, const std::stringstream& stream) {
  std::fstream output(path.c_str(), std::ios::out | std::ios::trunc);

  // TODO: If we cannot open more files, wait for some to finish and try again.
  if (!output.is_open()) {
    // LT_LOG("failed to open file for writing : path:%s", path.c_str());
    return false;
  }

  output << stream.rdbuf();

  if (!output.good()) {
    // LT_LOG("failed to write stream to file : path:%s", path.c_str());
    return false;
  }

  output.close();

  // Ensure that the new file is actually written to the disk
  int fd = ::open(path.c_str(), O_WRONLY);

  if (fd < 0) {
    // LT_LOG("failed to open file descriptor for fdatasync : path:%s", path.c_str());
    return false;
  }

  if (use_fsyncdisk) {
#ifdef __APPLE__
    ::fsync(fd);
#else
    ::fdatasync(fd);
#endif
  }

  ::close(fd);
  return true;
}

} // namespace anonymous

void
DownloadStorer::save_and_move_streams(const std::string& path, bool use_fsyncdisk,
                                       const std::stringstream& torrent_stream,
                                       const std::stringstream& rtorrent_stream,
                                       const std::stringstream& libtorrent_stream) {
  // LT_LOG("saving download : download:%p path:%s", download, path.c_str());

  auto torrent_path    = path;
  auto libtorrent_path = path + ".libtorrent_resume";
  auto rtorrent_path   = path + ".rtorrent";

  if (torrent_stream) {
    if (!save_stream(torrent_path + ".new", use_fsyncdisk, torrent_stream))
      return;
  }

  if (!save_stream(libtorrent_path + ".new", use_fsyncdisk, libtorrent_stream))
    return;

  if (!save_stream(rtorrent_path + ".new", use_fsyncdisk, rtorrent_stream))
    return;

  if (torrent_stream) {
    if (::rename((torrent_path + ".new").c_str(), torrent_path.c_str()) == -1) {
      // LT_LOG("failed to rename torrent file : %s", torrent_path.c_str());
      return;
    }
  }

  if (::rename((libtorrent_path + ".new").c_str(), libtorrent_path.c_str()) == -1) {
    // LT_LOG("failed to rename libtorrent resume file : %s", libtorrent_path.c_str());
    return;
  }

  if (::rename((rtorrent_path + ".new").c_str(), rtorrent_path.c_str()) == -1) {
    // LT_LOG("failed to rename rtorrent resume file : %s", rtorrent_path.c_str());
    return;
  }
}

utils::Directory
DownloadStorer::get_formated_entries(const std::string& session_path) {
  if (session_path.empty())
    return utils::Directory();

  utils::Directory d(session_path);

  if (!d.update(utils::Directory::update_hide_dot))
    throw torrent::storage_error("session::DownloadStorer::update() could not open session directory: " + session_path);

  d.erase(std::remove_if(d.begin(), d.end(), [](auto& entry) { return !is_correct_format(entry.s_name); }), d.end());
  return d;
}

} // namespace session
