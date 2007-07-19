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

#ifndef RAK_RANGES_H
#define RAK_RANGES_H

#include <algorithm>
#include <vector>
#include <rak/functional.h>

namespace rak {

template <typename Type>
class ranges : private std::vector<std::pair<Type, Type> > {
public:
  typedef std::vector<std::pair<Type, Type> >  base_type;
  typedef typename base_type::value_type       value_type;
  typedef typename base_type::reference        reference;
  typedef typename base_type::iterator         iterator;
  typedef typename base_type::const_iterator   const_iterator;
  typedef typename base_type::reverse_iterator reverse_iterator;

  using base_type::clear;
  using base_type::size;
  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::front;
  using base_type::back;

  void                insert(Type first, Type last) { insert(std::make_pair(first, last)); }
  void                erase(Type first, Type last)  { erase(std::make_pair(first, last)); }

  void                insert(value_type r);
  void                erase(value_type r);

  // Find the first ranges that has an end greater than index.
  iterator            find(Type index);
  const_iterator      find(Type index) const;

  // Use find with no closest match.
  bool                has(Type index) const;
};

template <typename Type>
void
ranges<Type>::insert(value_type r) {
  if (r.first >= r.second)
    return;

  iterator first = std::find_if(begin(), end(), rak::less_equal(r.first, rak::const_mem_ref(&value_type::second)));

  if (first == end() || r.second < first->first) {
    // The new range is before the first, after the last or between
    // two ranges.
    base_type::insert(first, r);

  } else {
    first->first = std::min(r.first, first->first);
    first->second = std::max(r.second, first->second);

    iterator last = std::find_if(first, end(), rak::less(first->second, rak::const_mem_ref(&value_type::second)));

    if (last != end() && first->second >= last->first)
      first->second = (last++)->second;

    base_type::erase(first + 1, last);
  }
}

template <typename Type>
void
ranges<Type>::erase(value_type r) {
  if (r.first >= r.second)
    return;

  iterator first = std::find_if(begin(), end(), rak::less(r.first, rak::const_mem_ref(&value_type::second)));
  iterator last  = std::find_if(first, end(), rak::less(r.second, rak::const_mem_ref(&value_type::second)));

  if (first == end())
    return;

  if (first == last) {

    if (r.first > first->first) {
      std::swap(first->first, r.second);
      base_type::insert(first, value_type(r.second, r.first));

    } else if (r.second > first->first) {
      first->first = r.second;
    }

  } else {

    if (r.first > first->first)
      (first++)->second = r.first;
    
    if (last != end() && r.second > last->first)
      last->first = r.second;

    base_type::erase(first, last);
  }
}

// Find the first ranges that has an end greater than index.
template <typename Type>
inline typename ranges<Type>::iterator
ranges<Type>::find(Type index) {
  return std::find_if(begin(), end(), rak::less(index, rak::const_mem_ref(&value_type::second)));
}

template <typename Type>
inline typename ranges<Type>::const_iterator
ranges<Type>::find(Type index) const {
  return std::find_if(begin(), end(), rak::less(index, rak::const_mem_ref(&value_type::second)));
}

// Use find with no closest match.
template <typename Type>
bool
ranges<Type>::has(Type index) const {
  const_iterator itr = find(index);

  return itr != end() && index >= itr->first;
}

}

#endif
