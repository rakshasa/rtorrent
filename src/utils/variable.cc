// rTorrent - BitTorrent client
// Copyright (C) 2006, Jari Sundell
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

#include <torrent/exceptions.h>

#include "variable.h"

namespace utils {

const torrent::Object Variable::m_emptyObject;

// Consider throwing an exception.
const torrent::Object&
Variable::get() {
  return m_emptyObject;
}

void
Variable::set(const torrent::Object& arg) {
}

const torrent::Object&
Variable::get_d(core::Download* download) {
  return get();
}

void
Variable::set_d(core::Download* download, const torrent::Object& arg) {
  set(arg);
}

const char*
Variable::string_to_value_unit(const char* pos, value_type* value, int base, int unit) {
  if (unit <= 0)
    throw torrent::input_error("Variable::string_to_value_unit(...) received unit <= 0.");

  char* last;
  *value = strtoll(pos, &last, base);

  if (last == pos) {
    if (strcasecmp(pos, "no") == 0) { *value = 0; return pos + strlen("no"); }
    if (strcasecmp(pos, "yes") == 0) { *value = 1; return pos + strlen("yes"); }
    if (strcasecmp(pos, "true") == 0) { *value = 1; return pos + strlen("true"); }
    if (strcasecmp(pos, "false") == 0) { *value = 0; return pos + strlen("false"); }

    throw torrent::input_error("Could not convert string to value.");
  }

  switch (*last) {
  case 'b':
  case 'B':
    ++last;
    break;

  case 'k':
  case 'K':
    *value = *value << 10;
    ++last;
    break;

  case 'm':
  case 'M':
    *value = *value << 20;
    ++last;
    break;

  case 'g':
  case 'G':
    *value = *value << 30;
    ++last;
    break;

  case ' ':
  case '\0':
    *value = *value * unit;
    break;

  default:
    throw torrent::input_error("Could not parse value.");
  }

  return last;
}

bool
Variable::string_to_value_unit_nothrow(const char* pos, value_type* value, int base, int unit) {
  try {
    string_to_value_unit(pos, value, base, unit);

    return true;
  } catch (const torrent::input_error& e) {
    return false;
  }
}

}
