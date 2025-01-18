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
#include <algorithm>

#include "canvas.h"
#include "globals.h"
#include "manager.h"
#include "window.h"

namespace display {

Manager::Manager() :
  m_forceRedraw(false) {

  m_taskUpdate.slot() = std::bind(&Manager::receive_update, this);
}

Manager::~Manager() {
  priority_queue_erase(&taskScheduler, &m_taskUpdate);
}

void
Manager::force_redraw() {
  m_forceRedraw = true;
}

void
Manager::schedule(Window* w, rak::timer t) {
  rak::priority_queue_update(&m_scheduler, w->task_update(), t);
  schedule_update(50000);
}

void
Manager::unschedule(Window* w) {
  rak::priority_queue_erase(&m_scheduler, w->task_update());
  schedule_update(50000);
}

void
Manager::adjust_layout() {
  Canvas::redraw_std();
  m_rootFrame.balance(0, 0, Canvas::get_screen_width(), Canvas::get_screen_height());

  schedule_update(0);
}

void
Manager::receive_update() {
  if (m_forceRedraw) {
    m_forceRedraw = false;

    display::Canvas::resize_term(display::Canvas::term_size());
    Canvas::redraw_std();

    adjust_layout();
    m_rootFrame.redraw();
  }

  Canvas::refresh_std();

  rak::priority_queue_perform(&m_scheduler, cachedTime);
  m_rootFrame.refresh();

  Canvas::do_update();

  m_timeLastUpdate = cachedTime;
  schedule_update(50000);
}

void
Manager::schedule_update(uint32_t minInterval) {
  if (m_scheduler.empty()) {
    rak::priority_queue_erase(&taskScheduler, &m_taskUpdate);
    return;
  }

  if (!m_taskUpdate.is_queued() || m_taskUpdate.time() > m_scheduler.top()->time()) {
    rak::priority_queue_update(&taskScheduler, &m_taskUpdate, std::max(m_scheduler.top()->time(), m_timeLastUpdate + minInterval));
  }
}

}
