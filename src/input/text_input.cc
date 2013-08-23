// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include "display/attributes.h"

#include "text_input.h"

namespace input {

bool
TextInput::pressed(int key) {
  if (m_bindings.pressed(key)) {
    return true;

  } else if (m_alt) {
    m_alt = false;

    switch (key) {
//     case 'b':
//       Base::insert(m_pos, "M^b");
//       break;

//     case 'f':
//       Base::insert(m_pos, "M^f");
//       break;

    default:
      return false;
    }

  } else if (key >= 0x20 && key < 0x7F) {
    Base::insert(m_pos++, 1, key);

  } else {
    switch (key) {
    case 0x7F:
    case 'h' - 'a' + 1: // ^H
    case KEY_BACKSPACE:
      if (m_pos != 0)
        Base::erase(--m_pos, 1);

      break;

    case KEY_DC:
      if (m_pos != size())
        Base::erase(m_pos, 1);

      break;

    case 0x02:
    case KEY_LEFT:
      if (m_pos != 0)
        --m_pos;

      break;

    case 0x06:
    case KEY_RIGHT:
      if (m_pos != size())
        ++m_pos;

      break;

    case KEY_HOME:
      m_pos = 0;
      break;

    case KEY_END:
      m_pos = size();
      break;

    case 'u' - 'a' + 1: // ^U
      Base::erase(0, m_pos);
      m_pos = 0;
      break;

    case 'k' - 'a' + 1: // ^K
      Base::erase(m_pos, size()-m_pos);
      break;

    case 0x1B:
      m_alt = true;

      break;

    default:
      return false;
    }
  }

  mark_dirty();

  return true;
}  

}
