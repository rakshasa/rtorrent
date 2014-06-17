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

#include <stdexcept>

#include "window.h"

namespace display {

Window::SlotTimer  Window::m_slotSchedule;
Window::SlotWindow Window::m_slotUnschedule;
Window::Slot       Window::m_slotAdjust;

// When constructing the window we set flag_offscreen so that redraw
// doesn't get triggered until after a successful Frame::balance call.

Window::Window(Canvas* canvas, int flags, extent_type minWidth, extent_type minHeight, extent_type maxWidth, extent_type maxHeight) :
  m_canvas(canvas),
  m_flags(flags | flag_offscreen),

  m_minWidth(minWidth),
  m_minHeight(minHeight),

  m_maxWidth(maxWidth),
  m_maxHeight(maxHeight) {

  m_taskUpdate.slot() = std::bind(&Window::redraw, this);
}

Window::~Window() {
  if (is_active())
    m_slotUnschedule(this);

  delete m_canvas;
}

void
Window::set_active(bool state) {
  if (state == is_active())
    return;

  if (state) {
    // Set offscreen so we don't try rendering before Frame::balance
    // has been called.
    m_flags |= flag_active | flag_offscreen;
    mark_dirty();

  } else {
    m_flags &= ~flag_active;
    m_slotUnschedule(this);
  }
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
