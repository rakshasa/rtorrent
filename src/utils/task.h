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

#ifndef RTORRENT_UTILS_TASK_H
#define RTORRENT_UTILS_TASK_H

#include <sigc++/slot.h>

#include "timer.h"
#include "task_schedule.h"

namespace utils {

class Task {
public:
  typedef sigc::slot<void> Slot;

  Task(Slot s = Slot()) : m_slot(s)       { clear_iterator(); }
  ~Task()                                 { remove(); }

  bool  is_scheduled()                    { return m_itr != TaskSchedule::end(); }

  void  set_slot(Slot s)                  { m_slot = s; }
  Slot& get_slot()                        { return m_slot; }

  Timer get_time()                        { return m_time; }

  void  insert(Timer t) {
    remove();

    m_time = t;
    m_itr = TaskSchedule::insert(this);
  }

  void  remove() {
    if (m_itr != TaskSchedule::end()) {
      TaskSchedule::erase(m_itr);
      clear_iterator();
    }
  }

protected:
  friend class TaskSchedule;

  TaskSchedule::iterator get_iterator()   { return m_itr; }
  void                   clear_iterator() { m_itr = TaskSchedule::end(); }

private:
  Task(const Task&);
  void operator () (const Task&);

  Timer                  m_time;
  TaskSchedule::iterator m_itr;
  Slot                   m_slot;
};

}

#endif
