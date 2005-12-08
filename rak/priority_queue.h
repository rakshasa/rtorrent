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

// priority_queue is a priority queue implemented using a binary
// heap. It can contain multiple instances of a value.

#ifndef RAK_PRIORITY_QUEUE_H
#define RAK_PRIORITY_QUEUE_H

#include <algorithm>
#include <functional>
#include <vector>

namespace rak {

template <typename Value, typename Compare, typename Equal, typename Remove>
class priority_queue : public std::vector<Value> {
public:
  typedef std::vector<Value>                  base_type;
  typedef typename base_type::reference       reference;
  typedef typename base_type::const_reference const_reference;
  typedef typename base_type::iterator        iterator;
  typedef typename base_type::const_iterator  const_iterator;
  typedef typename base_type::value_type      value_type;

  using base_type::size;
  using base_type::empty;

  priority_queue(Compare l = Compare(), Equal e = Equal(), Remove r = Remove())
    : m_compare(l), m_equal(e), m_remove(r) {}

  const_reference top() const {
    return base_type::front();
  }

  void pop() {
    std::pop_heap(base_type::begin(), base_type::end(), m_compare);
    base_type::pop_back();
  }

  void push(const value_type& value) {
    base_type::push_back(value);
    std::push_heap(base_type::begin(), base_type::end(), m_compare);
  }

  // Removes 'value' from the queue. The Remove functor must change the
  // priority of the value such that it comes before any other in the
  // queue.
  void erase(value_type value) {
    iterator itr = std::find_if(base_type::begin(), base_type::end(), std::bind2nd(m_equal, value));

    if (itr == base_type::end())
      return;

    m_remove(*itr);
    std::push_heap(base_type::begin(), ++itr, m_compare);
    pop();
  }

private:
  Compare             m_compare;
  Equal               m_equal;
  Remove              m_remove;
};

}

#endif
