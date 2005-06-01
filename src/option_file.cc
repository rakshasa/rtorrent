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
#include <fstream>
#include <stdexcept>

#include "option_file.h"

void
OptionFile::process(std::istream* stream) {
  char buf[max_size_line];

  while (stream->good()) {
    stream->getline(buf, max_size_line);

    parse_line(buf);
  }
}

void
OptionFile::parse_line(const char* line) {
  //const char* last = std::find(line, line + max_size_line, '\0');

  if (line[0] == '#')
    return;

  char key[64];
  char opt[512];

  int result;

  // Check for empty lines, and options within "abc".
  if ((result = std::sscanf(line, "%64s = %512s", key, opt)) == 2)
    m_slotOption(key, opt);

  // Don't throw on empty lines or lines with key and opt.
  if (result == 1)
    throw std::runtime_error("Error parseing option file.");
}
