// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <torrent/exceptions.h>

#include "canvas.h"

namespace display {

bool Canvas::m_isInitialized = false;

void
Canvas::resize(int x, int y, int w, int h) {
  wresize(m_window, h, w);
  mvwin(m_window, y, x);
}

void
Canvas::print_attributes(unsigned int x, unsigned int y, const char* first, const char* last, const attributes_list* attributes) {
  move(x, y);

  attributes_list::const_iterator attrItr = attributes->begin();
  Attributes current = Attributes(first, Attributes::a_normal, Attributes::color_default);

  while (first != last) {
    if (attrItr != attributes->end() && first >= attrItr->position())
      current = *attrItr++;

    waddch(m_window, *first++ | current.attributes());
  }

  // Reset the color.
}

void
Canvas::initialize() {
  if (m_isInitialized)
    return;
  
  m_isInitialized = true;

  initscr();
  raw();
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  curs_set(0);
}

void
Canvas::cleanup() {
  if (!m_isInitialized)
    return;
  
  m_isInitialized = false;

  noraw();
  endwin();
}

std::pair<int, int>
Canvas::term_size() {
  struct winsize ws;

  if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0)
    return std::pair<int, int>(ws.ws_col, ws.ws_row);
  else
    return std::pair<int, int>(80, 24);
}

}
