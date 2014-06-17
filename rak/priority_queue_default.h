// rak - Rakshasa's toolbox
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

#ifndef RAK_PRIORITY_QUEUE_DEFAULT_H
#define RAK_PRIORITY_QUEUE_DEFAULT_H

#include lt_tr1_functional
#include <rak/allocators.h>
#include <rak/priority_queue.h>
#include <rak/timer.h>

#include "torrent/exceptions.h"

namespace rak {

class priority_item {
public:
  typedef std::function<void (void)> slot_void;

  priority_item() {}
  ~priority_item() {
    if (is_queued())
      throw torrent::internal_error("priority_item::~priority_item() called on a queued item.");

    m_time = timer();
    m_slot = slot_void();
  }

  bool                is_valid() const              { return (bool)m_slot; }
  bool                is_queued() const             { return m_time != timer(); }

  slot_void&          slot()                        { return m_slot; }

  const timer&        time() const                  { return m_time; }
  void                clear_time()                  { m_time = timer(); }
  void                set_time(const timer& t)      { m_time = t; }

  bool                compare(const timer& t) const { return m_time > t; }

private:
  priority_item(const priority_item&);
  void operator = (const priority_item&);

  timer               m_time;
  slot_void           m_slot;
};

struct priority_compare {
  bool operator () (const priority_item* const p1, const priority_item* const p2) const {
    return p1->time() > p2->time();
  }
};

typedef std::equal_to<priority_item*> priority_equal;
typedef priority_queue<priority_item*, priority_compare, priority_equal,
                       cacheline_allocator<priority_item*> > priority_queue_default;

inline void
priority_queue_perform(priority_queue_default* queue, timer t) {
  while (!queue->empty() && queue->top()->time() <= t) {
    priority_item* v = queue->top();
    queue->pop();

    v->clear_time();
    v->slot()();
  }
}

inline void
priority_queue_insert(priority_queue_default* queue, priority_item* item, timer t) {
  if (t == timer())
    throw torrent::internal_error("priority_queue_insert(...) received a bad timer.");

  if (!item->is_valid())
    throw torrent::internal_error("priority_queue_insert(...) called on an invalid item.");

  if (item->is_queued())
    throw torrent::internal_error("priority_queue_insert(...) called on an already queued item.");

  if (queue->find(item) != queue->end())
    throw torrent::internal_error("priority_queue_insert(...) item found in queue.");

  item->set_time(t);
  queue->push(item);
}

inline void
priority_queue_erase(priority_queue_default* queue, priority_item* item) {
  if (!item->is_queued())
    return;

  // Check is_valid() after is_queued() so that it is safe to call
  // erase on untouched instances.
  if (!item->is_valid())
    throw torrent::internal_error("priority_queue_erase(...) called on an invalid item.");

  // Clear time before erasing to force it to the top.
  item->clear_time();
  
  if (!queue->erase(item))
    throw torrent::internal_error("priority_queue_erase(...) could not find item in queue.");

  if (queue->find(item) != queue->end())
    throw torrent::internal_error("priority_queue_erase(...) item still in queue.");
}

}

#endif
