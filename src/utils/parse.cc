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

#include <sstream>

#include "utils/parse.h"

namespace utils {

std::string
escape_string(const std::string& src) {
  std::stringstream stream;

  stream << std::hex << std::uppercase;

  for (std::string::const_iterator itr = src.begin(); itr != src.end(); ++itr)
    if ((*itr >= 'A' && *itr <= 'Z') ||
	(*itr >= 'a' && *itr <= 'z') ||
	(*itr >= '0' && *itr <= '9') ||
	*itr == '-')
      stream << *itr;
    else
      stream << '%' << ((unsigned char)*itr >> 4) << ((unsigned char)*itr & 0xf);

  return stream.str();
}

std::string
string_to_hex(const std::string& src) {
  std::stringstream stream;

  stream << std::hex << std::uppercase;

  for (std::string::const_iterator itr = src.begin(); itr != src.end(); ++itr)
    stream << ((unsigned char)*itr >> 4) << ((unsigned char)*itr & 0xf);

  return stream.str();
}

}
