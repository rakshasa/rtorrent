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
#include "utils.h"
#include "window_string_list.h"

namespace display {

WindowStringList::WindowStringList() :
  Window(new Canvas, true) {
}

WindowStringList::~WindowStringList() {
}

void
WindowStringList::redraw() {
  m_canvas->erase();

  size_t ypos = 0;
  size_t xpos = 1;
  size_t width = 0;

  iterator itr = m_first;

  while (itr != m_last) {

    if (ypos == (size_t)m_canvas->get_height()) {
      ypos = 0;
      xpos += width + 2;
      
      if (xpos + 20 >= (size_t)m_canvas->get_width())
	break;

      width = 0;
    }

    width = std::max(itr->size(), width);

    if (xpos + itr->size() <= (size_t)m_canvas->get_width())
      m_canvas->print(xpos, ypos++, "%s", itr->c_str());
    else
      m_canvas->print(xpos, ypos++, "%s", itr->substr(0, m_canvas->get_width() - xpos).c_str());

    ++itr;
  }

  m_drawEnd = itr;
}

}
