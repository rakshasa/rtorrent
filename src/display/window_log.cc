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

#include <ctime>

#include "canvas.h"
#include "utils.h"
#include "window_log.h"

namespace display {

WindowLog::WindowLog(core::Log* l) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_static),
  m_log(l) {

  m_taskUpdate.set_slot(rak::mem_fn(this, &WindowLog::receive_update)),

  // We're trying out scheduled tasks instead.
  m_connUpdate = l->signal_update().connect(sigc::mem_fun(*this, &WindowLog::receive_update));
}

WindowLog::~WindowLog() {
  priority_queue_erase(&taskScheduler, &m_taskUpdate);
  m_connUpdate.disconnect();
}

WindowLog::iterator
WindowLog::find_older() {
  return m_log->find_older(cachedTime - rak::timer::from_seconds(60));
}

void
WindowLog::redraw() {
  m_canvas->erase();

  int pos = m_canvas->height();

  for (core::Log::iterator itr = m_log->begin(), last = find_older(); itr != last && pos > 0; ++itr, --pos) {
    char buffer[16];
    print_hhmmss_local(buffer, buffer + 16, static_cast<time_t>(itr->first.seconds()));

    m_canvas->print(0, pos - 1, "(%s) %s", buffer, itr->second.c_str());
  }
}

// When WindowLog is activated, call receive_update() to ensure it
// gets updated.
void
WindowLog::receive_update() {
  if (!is_active())
    return;

  iterator itr = find_older();
  extent_type height = std::min(std::distance(m_log->begin(), itr), (std::iterator_traits<iterator>::difference_type)10);

  if (height != m_maxHeight) {
    m_minHeight = height != 0 ? 1 : 0;
    m_maxHeight = height;
    m_slotAdjust();

  } else {
    mark_dirty();
  }

  priority_queue_erase(&taskScheduler, &m_taskUpdate);

  if (height != 0)
    priority_queue_insert(&taskScheduler, &m_taskUpdate, (cachedTime + rak::timer::from_seconds(30)).round_seconds());
}

}
