// DownloadStore handles the saving and listing of session torrents.

#include "config.h"

#include <fstream>
#include <stdio.h>
#include <fcntl.h>
#include <sstream>
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

#include "download.h"
#include "download_store.h"
#include "rpc/parse_commands.h"
#include "session/session_manager.h"
#include "utils/directory.h"

namespace core {

bool
DownloadStore::save(Download* d, int flags) {
  if (!session_thread::manager()->is_used())
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

  auto download_stream = std::unique_ptr<std::stringstream>();
  auto resume_stream   = std::make_unique<std::stringstream>();
  auto rtorrent_stream = std::make_unique<std::stringstream>();

  if (!(flags & flag_skip_static)) {
    download_stream = std::make_unique<std::stringstream>();
    torrent::object_write_bencode(&*download_stream, d->bencode(), torrent::Object::flag_session_data);

    if (!download_stream->good())
      return false;
  }

  torrent::object_write_bencode(&*resume_stream, resume_base, 0);

  // TODO: Add logging.
  if (!resume_stream->good())
    return false;

  torrent::object_write_bencode(&*rtorrent_stream, rtorrent_base, 0);

  if (!rtorrent_stream->good())
    return false;

  auto base_filename = create_filename(d);

  session_thread::manager()->save_download(d, base_filename, std::move(download_stream), std::move(resume_stream), std::move(rtorrent_stream));
  return true;
}

void
DownloadStore::remove(Download* d) {
  session_thread::manager()->remove_download(d, create_filename(d));
}

// This also needs to check that it isn't a directory.
bool
not_correct_format(const utils::directory_entry& entry) {
  return !DownloadStore::is_correct_format(entry.s_name);
}

utils::Directory
DownloadStore::get_formated_entries() {
  if (!session_thread::manager()->is_used())
    return utils::Directory();

  utils::Directory d(session_thread::manager()->path());

  if (!d.update(utils::Directory::update_hide_dot))
    throw torrent::storage_error("core::DownloadStore::update() could not open session directory: " + session_thread::manager()->path());

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
  return session_thread::manager()->path() + rak::transform_hex(d->info()->hash().begin(), d->info()->hash().end()) + ".torrent";
}

}
