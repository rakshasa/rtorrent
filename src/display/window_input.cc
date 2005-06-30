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

#include "canvas.h"
#include "window_input.h"

#include "input/text_input.h"

namespace display {

WindowInput::WindowInput(input::TextInput* input) :
  Window(new Canvas, false, 1),
  m_input(input),
  m_focus(false) {
}

void
WindowInput::redraw() {
  m_canvas->erase();
  m_canvas->print(0, 0, "> %s", m_input->c_str());

  if (m_focus)
    m_canvas->set_attr(m_input->get_pos() + 2, 0, 1, A_REVERSE, COLOR_PAIR(0));
}

}
