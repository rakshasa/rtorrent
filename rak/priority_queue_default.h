// rak - Rakshasa's toolbox
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

#ifndef RAK_PRIORITY_QUEUE_DEFAULT_H
#define RAK_PRIORITY_QUEUE_DEFAULT_H

#include <stdexcept>
#include <rak/functional.h>
#include <rak/functional_fun.h>
#include <rak/priority_queue.h>
#include <rak/timer.h>

namespace rak {

class priority_item {
public:
  bool                is_queued() const             { return m_time != timer(); }

  void                call()                        { m_slot(); }
  void                set_slot(function<void> s)    { m_slot = s; }
  
  const timer&        time() const                  { return m_time; }

  priority_item*      prepare(const timer& t);
  priority_item*      clear()                       { m_time = timer(); return this; }

  bool                compare(const timer& t) const { return m_time > t; }

private:
  timer               m_time;
  function<void>      m_slot;
};

inline priority_item*
priority_item::prepare(const timer& t) {
  if (is_queued())
    throw std::logic_error("priority_item::prepare(rak::timer) called on an already queued item.");

  m_time = t;
  return this;
}

struct priority_compare {
  bool operator () (const priority_item* const p1, const priority_item* const p2) const {
    return p1->time() > p2->time();
  }
};

typedef std::equal_to<priority_item*> priority_equal;
typedef priority_queue<priority_item*, priority_compare, priority_equal> priority_queue_default;

}

#endif
