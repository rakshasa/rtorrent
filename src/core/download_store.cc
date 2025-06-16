// DownloadStore handles the saving and listing of session torrents.

#include "config.h"

#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <rak/error_number.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <torrent/utils/resume.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/object_stream.h>

#include "utils/directory.h"

#include "download.h"
#include "download_store.h"
#include "rpc/parse_commands.h"

namespace core {

void
DownloadStore::enable(bool lock) {
  if (is_enabled())
    throw torrent::input_error("Session directory already enabled.");

  if (m_path.empty())
    return;

  if (lock)
    m_lockfile.set_path(m_path + "rtorrent.lock");
  else
    m_lockfile.set_path(std::string());

  if (!m_lockfile.try_lock()) {
    if (rak::error_number::current().is_bad_path())
      throw torrent::input_error("Could not lock session directory: \"" + m_path + "\", " + rak::error_number::current().c_str());
    else
      throw torrent::input_error("Could not lock session directory: \"" + m_path + "\", held by \"" + m_lockfile.locked_by_as_string() + "\".");
  }
}

void
DownloadStore::disable() {
  if (!is_enabled())
    return;

  m_lockfile.unlock();
}

void
DownloadStore::set_path(const std::string& path) {
  if (is_enabled())
    throw torrent::input_error("Tried to change session directory while it is enabled.");

  if (!path.empty() && *path.rbegin() != '/')
    m_path = rak::path_expand(path + '/');
  else
    m_path = rak::path_expand(path);
}

bool
DownloadStore::write_bencode(const std::string& filename, const torrent::Object& obj, uint32_t skip_mask) {
  int fd;
  torrent::Object tmp;
  std::fstream output(filename.c_str(), std::ios::out | std::ios::trunc);

  if (!output.is_open())
    goto download_store_save_error;

  torrent::object_write_bencode(&output, &obj, skip_mask);

  if (!output.good())
    goto download_store_save_error;

  output.close();

  // Test the new file, to ensure it is a valid bencode string.
  output.open(filename.c_str(), std::ios::in);
  output >> tmp;

  if (!output.good())
    goto download_store_save_error;

  output.close();

  // Ensure that the new file is actually written to the disk
  fd = ::open(filename.c_str(), O_WRONLY);
  if (fd < 0)
    goto download_store_save_error;

  if (rpc::call_command_value("system.files.session.fdatasync")) {
#ifdef __APPLE__
    fsync(fd);
#else
    fdatasync(fd);
#endif
  }

  ::close(fd);

  return true;

download_store_save_error:
  output.close();
  return false;
}

bool
DownloadStore::save(Download* d, int flags) {
  if (!is_enabled())
    return true;

  torrent::Object* resume_base   = &d->download()->bencode()->get_key("libtorrent_resume");
  torrent::Object* rtorrent_base = &d->download()->bencode()->get_key("rtorrent");

  // Move this somewhere else?
  rtorrent_base->insert_key("chunks_done",    d->download()->file_list()->completed_chunks());
  rtorrent_base->insert_key("chunks_wanted",  d->download()->data()->wanted_chunks());
  rtorrent_base->insert_key("total_uploaded", d->info()->up_rate()->total());
  rtorrent_base->insert_key("total_downloaded", d->info()->down_rate()->total());

  // Don't save for completed torrents when we've cleared the uncertain_pieces.
  torrent::resume_save_progress(*d->download(), *resume_base);
  torrent::resume_save_uncertain_pieces(*d->download(), *resume_base);

  torrent::resume_save_addresses(*d->download(), *resume_base);
  torrent::resume_save_file_priorities(*d->download(), *resume_base);
  torrent::resume_save_tracker_settings(*d->download(), *resume_base);

  // Temp fixing of all flags, move to a better place:
  resume_base->set_flags(torrent::Object::flag_session_data);
  rtorrent_base->set_flags(torrent::Object::flag_session_data);

  std::string base_filename = create_filename(d);

  if (!write_bencode(base_filename + ".libtorrent_resume.new", *resume_base, 0) ||
      !write_bencode(base_filename + ".rtorrent.new", *rtorrent_base, 0))
    return false;

  ::rename((base_filename + ".libtorrent_resume.new").c_str(), (base_filename + ".libtorrent_resume").c_str());
  ::rename((base_filename + ".rtorrent.new").c_str(), (base_filename + ".rtorrent").c_str());

  if (!(flags & flag_skip_static) &&
      write_bencode(base_filename + ".new", *d->bencode(), torrent::Object::flag_session_data))
    ::rename((base_filename + ".new").c_str(), base_filename.c_str());

  return true;
}

void
DownloadStore::remove(Download* d) {
  if (!is_enabled())
    return;

  ::unlink((create_filename(d) + ".libtorrent_resume").c_str());
  ::unlink((create_filename(d) + ".rtorrent").c_str());
  ::unlink(create_filename(d).c_str());
}

// This also needs to check that it isn't a directory.
bool
not_correct_format(const utils::directory_entry& entry) {
  return !DownloadStore::is_correct_format(entry.s_name);
}

utils::Directory
DownloadStore::get_formated_entries() {
  if (!is_enabled())
    return utils::Directory();

  utils::Directory d(m_path);

  if (!d.update(utils::Directory::update_hide_dot))
    throw torrent::storage_error("core::DownloadStore::update() could not open directory \"" + m_path + "\"");

  d.erase(std::remove_if(d.begin(), d.end(), [&](const utils::directory_entry& entry) { return not_correct_format(entry); }), d.end());

  return d;
}

bool
DownloadStore::is_correct_format(const std::string& f) {
  if (f.size() != 48 || f.substr(40) != ".torrent")
    return false;

  for (std::string::const_iterator itr = f.begin(); itr != f.end() - 8; ++itr)
    if (!(*itr >= '0' && *itr <= '9') &&
        !(*itr >= 'A' && *itr <= 'F'))
      return false;

  return true;
}

std::string
DownloadStore::create_filename(Download* d) {
  return m_path + rak::transform_hex(d->info()->hash().begin(), d->info()->hash().end()) + ".torrent";
}

}
