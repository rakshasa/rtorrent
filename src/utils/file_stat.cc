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

#include <stdexcept>

#include "file_stat.h"

namespace utils {

void
FileStat::update_throws(int fd) {
  int r = update(fd);

  if (r < 0)
    throw std::runtime_error(error_string(r));
}

void
FileStat::update_throws(const char* filename) {
  int r = update(filename);

  if (r < 0)
    throw std::runtime_error(error_string(r));
}

std::string
FileStat::error_string(int err) {
  switch (err) {
  case 0:
    return "Success";

  case EBADF:
    return "Bad file descriptor";

  case ENOENT:
    return "Filename does not exist";

  case ENOTDIR:
    return "Path not a directory";

  case EACCES:
    return "Permission denied";

  default:
    return "Unknown error";
  }
}

}
