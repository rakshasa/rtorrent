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

void
TaskScheduler::insert(TaskItem* task, rak::timer time) {
  if (is_scheduled(task))
    throw std::logic_error("TaskScheduler::insert(...) tried to insert an already inserted or invalid TaskItem");

  // Only insert at or after m_entry because if we might be in
  // execute(...).
  iterator itr = std::find_if(m_entry, end(), rak::less_equal(time, rak::mem_ptr_ref(&value_type::first)));

  task->set_iterator(Base::insert(itr, value_type(time, task)));

  // Make sure m_entry points to the right node if we try inserting
  // before m_entry.
  if (itr == m_entry)
    m_entry = task->get_iterator();
}

void
TaskScheduler::erase(TaskItem* task) {
  if (!is_scheduled(task))
    return;

  iterator itr = Base::erase(task->get_iterator());

  if (task->get_iterator() == m_entry)
    m_entry = itr;

  task->set_iterator(end());
}

void
TaskScheduler::execute(rak::timer time) {
  m_entry = std::find_if(begin(), end(), rak::less(time, rak::mem_ptr_ref(&value_type::first)));

  // Since we are always using the front rather than a splice of the
  // due tasks, it is safe to erase them from within other tasks.
  while (begin() != m_entry) {
    if (!is_scheduled(Base::front().second))
      throw std::logic_error("TaskScheduler::execute_task(iterator) received an invalid iterator");
    
    Base::front().second->set_iterator(end());
    Base::front().second->get_slot()();

    Base::pop_front();
  }
}

}
