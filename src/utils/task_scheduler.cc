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

#include "config.h"

#include <stdexcept>

#include "rak/functional.h"
#include "task_scheduler.h"

namespace utils {

inline void
TaskScheduler::execute_task(const value_type& v) {
  if (!is_scheduled(v.second))
    throw std::logic_error("TaskScheduler::execute_task(iterator) received an invalid iterator");

  v.second->set_iterator(end());
  v.second->get_slot()();
}

void
TaskScheduler::insert(TaskItem* task, Timer time) {
  if (is_scheduled(task))
    throw std::logic_error("TaskScheduler::insert(...) tried to insert an already inserted or invalid TaskItem");

  iterator itr = std::find_if(begin(), end(),
			      rak::less_equal(time, rak::mem_ptr_ref(&value_type::first)));

  task->set_iterator(Base::insert(itr, value_type(time, task)));
}

void
TaskScheduler::erase(TaskItem* task) {
  if (!is_scheduled(task))
    return;

  Base::erase(task->get_iterator());
  task->set_iterator(end());
}

void
TaskScheduler::execute(Timer time) {
  Base tmp;

  tmp.splice(tmp.begin(), *this,
	     begin(), std::find_if(begin(), end(), rak::less_equal(time, rak::mem_ptr_ref(&value_type::first))));

  std::for_each(tmp.begin(), tmp.end(),
		rak::bind1st(std::mem_fun(&TaskScheduler::execute_task), this));
}

}
