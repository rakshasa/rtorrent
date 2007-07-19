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

#include <torrent/exceptions.h>

#include "command_scheduler_item.h"

namespace rpc {

CommandSchedulerItem::~CommandSchedulerItem() {
  priority_queue_erase(&taskScheduler, &m_task);
}

void
CommandSchedulerItem::enable(rak::timer t) {
  if (t == rak::timer())
    throw torrent::internal_error("CommandSchedulerItem::enable() t == rak::timer().");

  if (is_queued())
    disable();

  // If 'first' is zero then we execute the task
  // immediately. ''interval()'' will not return zero so we never end
  // up in an infinit loop.
  m_timeScheduled = t;
  priority_queue_insert(&taskScheduler, &m_task, t);
}

void
CommandSchedulerItem::disable() {
  m_timeScheduled = rak::timer();
  priority_queue_erase(&taskScheduler, &m_task);
}

rak::timer
CommandSchedulerItem::next_time_scheduled() const {
  if (m_interval == 0)
    return rak::timer();

  if (m_timeScheduled == rak::timer())
    throw torrent::internal_error("CommandSchedulerItem::next_time_scheduled() m_timeScheduled == rak::timer().");

  rak::timer next = m_timeScheduled;

  // This should be done in a non-looping manner.
  do {
    next += rak::timer::from_seconds(m_interval);
  } while (next <= cachedTime.round_seconds());

  return next;
}

}
