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

#ifndef RTORRENT_WINDOW_BASE_H
#define RTORRENT_WINDOW_BASE_H

#include <sigc++/slot.h>

#include "canvas.h"
#include "utils/task.h"
#include "utils/timer.h"

namespace display {

class Canvas;

class Window {
public:
  typedef sigc::slot0<void> Slot;

  Window(Canvas* c = NULL, bool d = false, int h = 1);

  virtual ~Window();

  bool                is_active()                          { return m_active; }
  bool                is_dynamic()                         { return m_dynamic; }
  bool                is_dirty()                           { return utils::displayScheduler.is_scheduled(&m_taskUpdate); }

  //utils::Timer        get_next_draw()                      { return m_nextDraw; }

  int                 get_min_height()                     { return m_minHeight; }

  bool                get_active()                         { return m_active; }
  void                set_active(bool a);

  void                refresh()                            { m_canvas->refresh(); }
  void                resize(int x, int y, int w, int h);

  void                mark_dirty();

  virtual void        redraw() = 0;

  static void         slot_adjust(Slot s)                  { m_slotAdjust = s; }

protected:
  Window(const Window&);
  void operator = (const Window&);

  static Slot         m_slotAdjust;

  Canvas*             m_canvas;

  bool                m_active;
  bool                m_dynamic;
  int                 m_minHeight;

  utils::TaskItem     m_taskUpdate;
};

inline void
Window::mark_dirty() {
  utils::displayScheduler.erase(&m_taskUpdate);
  utils::displayScheduler.insert(&m_taskUpdate, utils::Timer::cache());
}

}

#endif

