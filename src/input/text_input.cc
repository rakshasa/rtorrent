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
#include <ncurses.h>

#include "text_input.h"

namespace input {

bool
TextInput::pressed(int key) {
  //std::stringstream str;

  if (m_alt) {
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

    case 0x1B:
      m_alt = true;

      break;

    default:
      return false;

      // Testcode.
//       if (key == KEY_ENTER || key == '\n')
// 	return false;

//       str << "\\x" << std::hex << key;

//       Base::insert(m_pos, str.str());

//       m_pos += str.str().length();

//       return true;
    }
  }

  m_slotDirty();

  return true;

}  

}
