// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
#include <rak/functional.h>

#include "canvas.h"
#include "globals.h"
#include "manager.h"
#include "window.h"

namespace display {

Manager::Manager() :
  m_forceRedraw(false) {
  m_taskUpdate.set_slot(rak::mem_fn(this, &Manager::receive_update));
}

Manager::~Manager() {
  priority_queue_erase(&taskScheduler, &m_taskUpdate);
}

void
Manager::force_redraw() {
  m_forceRedraw = true;
}


Manager::iterator
Manager::insert(iterator pos, Window* w) {
  return Base::insert(pos, w);
}

// Swap with the function below.
Manager::iterator
Manager::erase(iterator pos) {
  if (pos != end())
    return erase(*pos);
  else
    return end();
}

Manager::iterator
Manager::erase(Window* w) {
  iterator itr = std::find(begin(), end(), w);

  if (itr == end())
    throw std::logic_error("Manager::erase(...) did not find the window");

  return Base::erase(itr);
}

Manager::iterator
Manager::find(Window* w) {
  return std::find(begin(), end(), w);
}

void
Manager::schedule(Window* w, rak::timer t) {
  rak::priority_queue_erase(&m_scheduler, w->task_update());
  rak::priority_queue_insert(&m_scheduler, w->task_update(), t);
  schedule_update();
}

void
Manager::unschedule(Window* w) {
  rak::priority_queue_erase(&m_scheduler, w->task_update());
  schedule_update();
}

void
Manager::adjust_layout() {
  int staticHeight = std::for_each(begin(), end(),
				   rak::if_then(std::mem_fun(&Window::is_active),
						rak::accumulate(0, std::mem_fun(&Window::get_min_height)))).m_then.result;
  int countDynamic = std::for_each(begin(), end(),
				   rak::if_then(std::mem_fun(&Window::is_active),
						rak::accumulate(0, std::mem_fun(&Window::is_dynamic)))).m_then.result;

  int dynamic = std::max(0, Canvas::get_screen_height() - staticHeight);
  int height = 0, h;

  for (iterator itr = begin(); itr != end(); ++itr, height += h) {
    h = 0;

    if (!(*itr)->is_active())
      continue;

    if ((*itr)->is_dynamic()) {
      dynamic -= h = (dynamic + countDynamic - 1) / countDynamic;
      countDynamic--;
    } else {
      h = 0;
    }

    h += (*itr)->get_min_height();

    (*itr)->resize(0, height, Canvas::get_screen_width(), h);
    (*itr)->mark_dirty();
  }
}

void
Manager::receive_update() {
  if (m_forceRedraw) {
    m_forceRedraw = false;

    display::Canvas::resize_term(display::Canvas::term_size());
    Canvas::redraw_std();
    adjust_layout();
  }

  Canvas::refresh_std();

  rak::priority_queue_perform(&m_scheduler, cachedTime);
  std::for_each(begin(), end(), rak::if_then(std::mem_fun(&Window::is_active), std::mem_fun(&Window::refresh)));

  Canvas::do_update();

  m_timeLastUpdate = cachedTime;
  schedule_update();
}

void
Manager::schedule_update() {
  if (m_scheduler.empty()) {
    rak::priority_queue_erase(&taskScheduler, &m_taskUpdate);
    return;
  }

  if (!m_taskUpdate.is_queued() || m_taskUpdate.time() > m_scheduler.top()->time()) {
    rak::priority_queue_erase(&taskScheduler, &m_taskUpdate);
    rak::priority_queue_insert(&taskScheduler, &m_taskUpdate,
			       std::max(m_scheduler.top()->time(), m_timeLastUpdate + 50000));
  }
}

}
