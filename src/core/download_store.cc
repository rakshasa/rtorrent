// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

// DownloadStore handles the saving and listing of session torrents.

#include "config.h"

#include <fstream>
#include <stdio.h>
#include <unistd.h>
#include <rak/error_number.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/resume.h>
#include <torrent/object_stream.h>

#include "download.h"
#include "download_store.h"

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

  if (!m_lockfile.try_lock())
    if (rak::error_number::current().is_bad_path())
      throw torrent::input_error("Could not lock session directory: \"" + m_path + "\", " + rak::error_number::current().c_str());
    else
      throw torrent::input_error("Could not lock session directory: \"" + m_path + "\", held by \"" + m_lockfile.locked_by_as_string() + "\".");
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

void
DownloadStore::save(Download* d) {
  if (!is_enabled())
    return;

  std::fstream f((create_filename(d) + ".new").c_str(), std::ios::out | std::ios::trunc);

  if (!f.is_open())
    return;

  // Move this somewhere else?
  d->bencode()->get_key("rtorrent").insert_key("total_uploaded", d->download()->up_rate()->total());
  d->bencode()->get_key("rtorrent").insert_key("chunks_done", d->download()->file_list()->completed_chunks());

  torrent::Object& resumeObject = d->download()->bencode()->get_key("libtorrent_resume");

  torrent::resume_save_addresses(*d->download(), resumeObject);
  torrent::resume_save_file_priorities(*d->download(), resumeObject);
  torrent::resume_save_tracker_settings(*d->download(), resumeObject);

  f << *d->bencode();

  if (!f.good())
    return;

  f.close();

  // Test the new file, to ensure it is a valid bencode string.
  f.open((create_filename(d) + ".new").c_str(), std::ios::in);

  torrent::Object tmp;
  f >> tmp;

  if (!f.good())
    return;

  f.close();

  ::rename((create_filename(d) + ".new").c_str(), create_filename(d).c_str());
}

void
DownloadStore::remove(Download* d) {
  if (!is_enabled())
    return;

  ::unlink(create_filename(d).c_str());
}

utils::Directory
DownloadStore::get_formated_entries() {
  if (!is_enabled())
    return utils::Directory();

  utils::Directory d(m_path);

  if (!d.update())
    throw torrent::storage_error("core::DownloadStore::update() could not open directory \"" + m_path + "\"");

  d.erase(std::remove_if(d.begin(), d.end(), std::not1(std::ptr_fun(&DownloadStore::is_correct_format))), d.end());

  return d;
}

bool
DownloadStore::is_correct_format(std::string f) {
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
  return m_path + rak::transform_hex(d->download()->info_hash().begin(), d->download()->info_hash().end()) + ".torrent";
}

}
