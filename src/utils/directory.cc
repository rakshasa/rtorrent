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

#include "config.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <dirent.h>
#include <rak/path.h>

#include "directory.h"

namespace utils {

// Keep this?
bool
Directory::is_valid() const {
  if (m_path.empty())
    return false;

  DIR* d = opendir(rak::path_expand(m_path).c_str());
  closedir(d);

  return d;
}

// Update should take various flags and sort functors.
bool
Directory::update(bool hideDot) {
  if (m_path.empty())
    throw std::logic_error("Directory::update() tried to open an empty path");

  DIR* d = opendir(rak::path_expand(m_path).c_str());

  if (d == NULL)
    return false;

  struct dirent* ent;

  // Err... let us use getdirentries here instead.
  while ((ent = readdir(d)) != NULL) {
    // Don't construct it here, check the const char.
    std::string de(ent->d_name);

    if (!de.empty() && (!hideDot || de[0] != '.')) {
      iterator itr = base_type::insert(end(), value_type());

      itr->d_fileno = ent->d_fileno;
      itr->d_reclen = ent->d_reclen;
      itr->d_type   = ent->d_type;
      itr->d_name   = de;
    }
  }

  closedir(d);
  std::sort(begin(), end());

  return true;
}

std::vector<std::string>
Directory::make_list() {
  std::vector<std::string> l;
  l.reserve(size());

  for (iterator itr = begin(); itr != end(); ++itr)
    l.push_back(m_path + itr->d_name);

  return l;
}

}
