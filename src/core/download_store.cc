// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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

#include "config.h"

#include <fstream>
#include <stdexcept>
#include <unistd.h>
#include <torrent/bencode.h>
#include <torrent/torrent.h>

#include "utils/parse.h"

#include "download.h"
#include "download_store.h"

namespace core {

void
DownloadStore::use(const std::string& path) {
  m_path = path;

  if (!m_path.empty() && *m_path.rbegin() != '/')
    m_path += '/';
}

void
DownloadStore::save(Download* d) {
  if (!is_active())
    return;

  std::fstream f(create_filename(d).c_str(), std::ios::out | std::ios::trunc);

  if (!f.is_open())
    throw std::runtime_error("core::DownloadStore::save(...) could not open file");

  f << d->get_bencode();

  if (f.fail())
    throw std::runtime_error("core::DownloadStore::save(...) could not write torrent");
}

void
DownloadStore::remove(Download* d) {
  if (!is_active())
    return;

  unlink(create_filename(d).c_str());
}

utils::Directory
DownloadStore::get_formated_entries() {
  if (!is_active())
    return utils::Directory();

  utils::Directory d(m_path);

  if (!d.update())
    throw std::runtime_error("core::DownloadStore::update() could not open directory \"" + m_path + "\"");

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
  return m_path + utils::string_to_hex(d->get_hash()) + ".torrent";
}

}
