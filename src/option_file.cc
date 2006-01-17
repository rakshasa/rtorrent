// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
#include <stdio.h>
#include <torrent/exceptions.h>

#include "option_file.h"

bool
OptionFile::process_file(const std::string& filename) {
  std::fstream file(filename.c_str(), std::ios::in);

  if (!file.good())
    return false;

  int lineNumber = 0;
  char buffer[max_size_line];

  try {

    while (file.getline(buffer, max_size_line).good()) {
      lineNumber++;
      m_slotOption(buffer);
    }

  } catch (torrent::input_error& e) {
    snprintf(buffer, max_size_line, "Error in option file: %s:%i: %s", filename.c_str(), lineNumber, e.what());

    throw std::runtime_error(buffer);
  }

  return true;
}

// void
// OptionFile::parse_line(const char* line) {
  //const char* last = std::find(line, line + max_size_line, '\0');

//   if (line[0] == '#')
//     return;

//   int result;
//   char key[64];
//   char opt[512];

//   opt[0] = '\0';

//   // Check for empty lines, and options within "abc".
//   if ((result = std::sscanf(line, "%63s = \"%511[^\"]", key, opt)) != 2 &&
//       (result = std::sscanf(line, "%63s = %511s", key, opt)) != 2 &&
//       result == 1)
//     throw torrent::input_error("Error parseing option file.");

//   if (opt[0] == '"' && opt[1] == '"')
//     opt[0] = '\0';

//   if (result >= 1)
//     m_slotOption(key, opt);
// }
