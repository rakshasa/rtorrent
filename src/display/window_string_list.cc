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
