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

#include "config.h"

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <dirent.h>

#include "directory.h"

namespace utils {

void
Directory::update() {
  if (m_path.empty())
    throw std::logic_error("Directory::update() tried to open an empty path");

  DIR* d = opendir(m_path.c_str());

  if (d == NULL)
    throw std::runtime_error("Directory::update() could not open directory");

  struct dirent* ent;

  while ((ent = readdir(d)) != NULL) {
    std::string de(ent->d_name);

    if (de != "." && de != "..")
      Base::push_back(ent->d_name);
  }

  closedir(d);
  Base::sort(std::less<std::string>());
}

Directory::Base
Directory::make_list() {
  Base l;

  for (Base::iterator itr = begin(); itr != end(); ++itr)
    l.push_back(m_path + *itr);

  return l;
}

}
