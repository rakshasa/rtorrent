#include "config.h"

#include <ncurses.h>

#include "text_input.h"
#include <sstream>

namespace input {

bool
TextInput::pressed(int key) {
  if (m_alt) {
    m_alt = false;

    switch (key) {
    case 'p':
      Base::insert(m_pos, "M^p");
      break;

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

    case KEY_LEFT:
    case 0x10:
      if (m_pos != 0)
	--m_pos;

      break;

    case KEY_RIGHT:
    case 0x0E:
      if (m_pos != size())
	++m_pos;

      break;

    default:
      return false;
    }
  }

  m_slotDirty();

  return true;

  // Testcode.
//   if (key == KEY_ENTER || key == '\n')
//     return false;

//   std::stringstream str;

//   str << "\\x" << std::hex << key;

//   Base::insert(m_pos, str.str());

//   m_pos += str.str().length();

//   return true;
}  

}
