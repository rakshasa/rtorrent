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

#include <algorithm>
#include <functional>

#include "task.h"
#include "task_schedule.h"

namespace utils {

TaskSchedule::Container TaskSchedule::m_container;

// Remove this, replace with stuff.
struct task_comp {
  task_comp(Timer t) : m_time(t) {}

  bool operator () (Task* t) {
    return m_time <= t->get_time();
  }

  Timer m_time;
};

inline void
TaskSchedule::execute_task(Task* t) {
  t->clear_iterator();
  t->get_slot()();
}

void
TaskSchedule::perform(Timer t) {
  Container c;

  c.splice(c.begin(), c, m_container.begin(), std::find_if(m_container.begin(), m_container.end(), task_comp(t)));

  std::for_each(c.begin(), c.end(), std::ptr_fun(&TaskSchedule::execute_task));
}

Timer
TaskSchedule::get_timeout() {
  if (!m_container.empty())
    return std::max(m_container.front()->get_time() - Timer::current(), Timer());
  else
    return Timer((int64_t)(1 << 30) * 1000000);
}

TaskSchedule::iterator
TaskSchedule::insert(Task* t) {
  iterator itr = std::find_if(m_container.begin(), m_container.end(), task_comp(t->get_time()));

  return m_container.insert(itr, t);
}
			       
}
