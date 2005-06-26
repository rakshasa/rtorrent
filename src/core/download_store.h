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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_CORE_DOWNLOAD_STORE_H
#define RTORRENT_CORE_DOWNLOAD_STORE_H

#include <string>

#include "utils/directory.h"

namespace core {

class Download;

class DownloadStore {
public:

  // Disable by passing an empty string.
  void              use(const std::string& path);

  bool              is_active() { return !m_path.empty(); }

  void              save(Download* d);
  void              remove(Download* d);

  // Currently shows all entries in the correct format.
  utils::Directory  get_formated_entries();

private:
  static bool       is_correct_format(std::string f);
  std::string       create_filename(Download* d);

  std::string       m_path;
};

}

#endif
