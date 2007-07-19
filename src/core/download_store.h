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

#ifndef RTORRENT_CORE_DOWNLOAD_STORE_H
#define RTORRENT_CORE_DOWNLOAD_STORE_H

#include <string>

#include "utils/directory.h"
#include "utils/lockfile.h"

namespace core {

class Download;

class DownloadStore {
public:

  bool                is_enabled()                            { return m_lockfile.is_locked(); }

  void                enable(bool lock);
  void                disable();

  const std::string&  path() const                            { return m_path; }
  void                set_path(const std::string& path);

  void                save(Download* d);
  void                remove(Download* d);

  // Currently shows all entries in the correct format.
  utils::Directory    get_formated_entries();

private:
  static bool         is_correct_format(std::string f);
  std::string         create_filename(Download* d);

  std::string         m_path;
  utils::Lockfile     m_lockfile;
};

}

#endif
