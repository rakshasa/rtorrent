#include "config.h"

#include <ncurses.h>

#include "text_input.h"
#include <sstream>

namespace input {

bool
TextInput::pressed(int key) {
  std::stringstream str;

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
      //return false;

      // Testcode.
      if (key == KEY_ENTER || key == '\n')
	return false;

      str << "\\x" << std::hex << key;

      Base::insert(m_pos, str.str());

      m_pos += str.str().length();

      return true;
    }
  }

  m_slotDirty();

  return true;

}  

}
