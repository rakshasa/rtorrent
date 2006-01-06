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

#ifndef RTORRENT_WINDOW_BASE_H
#define RTORRENT_WINDOW_BASE_H

#include <rak/timer.h>
#include <rak/functional.h>

#include "canvas.h"
#include "globals.h"

namespace display {

class Canvas;
class Manager;

class Window {
public:
  typedef rak::mem_fun0<Manager, void>                      Slot;
  typedef rak::mem_fun1<Manager, void, Window*>             SlotWindow;
  typedef rak::mem_fun2<Manager, void, Window*, rak::timer> SlotTimer;

  Window(Canvas* c = NULL, bool d = false, int h = 1);

  virtual ~Window();

  bool                is_active()                          { return m_active; }
  bool                is_dynamic()                         { return m_dynamic; }
  bool                is_dirty()                           { return m_taskUpdate.is_queued(); }

  int                 get_min_height()                     { return m_minHeight; }

  bool                get_active()                         { return m_active; }
  void                set_active(bool a);

  void                refresh()                            { m_canvas->refresh(); }
  void                resize(int x, int y, int w, int h);

  void                mark_dirty()                         { m_slotSchedule(this, cachedTime + 1); }

  virtual void        redraw() = 0;

  rak::priority_item* task_update()                        { return &m_taskUpdate; }

  // Slot for adjust and refresh.
  static void         slot_schedule(SlotTimer s)           { m_slotSchedule = s; }
  static void         slot_unschedule(SlotWindow s)        { m_slotUnschedule = s; }
  static void         slot_adjust(Slot s)                  { m_slotAdjust = s; }

protected:
  Window(const Window&);
  void operator = (const Window&);

  static SlotTimer    m_slotSchedule;
  static SlotWindow   m_slotUnschedule;
  static Slot         m_slotAdjust;

  Canvas*             m_canvas;

  bool                m_active;
  bool                m_dynamic;
  int                 m_minHeight;

  rak::priority_item  m_taskUpdate;
};

}

#endif

