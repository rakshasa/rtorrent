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

#ifndef RTORRENT_DISPLAY_WINDOW_INPUT_H
#define RTORRENT_DISPLAY_WINDOW_INPUT_H

#include "window.h"

namespace input {
  class TextInput;
}

namespace display {

class WindowInput : public Window {
public:
  WindowInput(input::TextInput* input);

  input::TextInput* get_input()       { return m_input; }

  bool              get_focus()       { return m_focus; }
  void              set_focus(bool f) { mark_dirty(); m_focus = f; }

  virtual void      redraw();

private:
  input::TextInput* m_input;
  bool              m_focus;
};

}

#endif
