// rTorrent - BitTorrent library
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

#ifndef RTORRENT_UTILS_TASK_SCHEDULER_H
#define RTORRENT_UTILS_TASK_SCHEDULER_H

#include "task_item.h"

namespace utils {

class TaskScheduler : private std::list<std::pair<Timer, TaskItem*> > {
public:
  typedef std::list<std::pair<Timer, TaskItem*> > Base;

  using Base::value_type;
  using Base::reference;

  using Base::iterator;
  using Base::reverse_iterator;
  using Base::size;
  using Base::empty;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  void                insert(TaskItem* task, Timer time);
  void                erase(TaskItem* task);

  void                execute(Timer time);

  bool                is_scheduled(TaskItem* task)        { return task->get_iterator() != end(); }

  Timer               get_next_timeout() const            { return begin()->first; }

private:
  inline void         execute_task(const value_type& v);
};

}

#endif
