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

#include <stdexcept>

#include "window.h"

namespace display {

Window::Slot Window::m_slotAdjust;

Window::Window(Canvas* c, bool d, int h) :
  m_canvas(c),
  m_active(true),
  m_dynamic(d),
  m_minHeight(h) {

  m_taskUpdate.set_iterator(utils::displayScheduler.end());
  m_taskUpdate.set_slot(sigc::mem_fun(*this, &Window::redraw));
}

Window::~Window() {
  utils::displayScheduler.erase(&m_taskUpdate);
  delete m_canvas;
}

void
Window::set_active(bool a) {
  if (a)
    mark_dirty();
  else
    utils::displayScheduler.erase(&m_taskUpdate);

  m_active = a;
}

void
Window::resize(int x, int y, int w, int h) {
  if (x < 0 || y < 0)
    throw std::logic_error("Window::resize(...) bad x or y position");

  if (w <= 0 || h <= 0)
    throw std::logic_error("Window::resize(...) bad size");

  m_canvas->resize(x, y, w, h);
}

}
