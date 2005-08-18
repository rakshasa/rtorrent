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

  TaskScheduler() : m_entry(begin()) {}

  void                insert(TaskItem* task, Timer time);
  void                erase(TaskItem* task);

  void                execute(Timer time);

  bool                is_scheduled(const TaskItem* task) const { return task->get_iterator() != end(); }

  Timer               get_next_timeout() const                 { return begin()->first; }

private:
  iterator            m_entry;
};

}

#endif
