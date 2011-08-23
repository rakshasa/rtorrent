// rTorrent - BitTorrent client
// Copyright (C) 2005-2008, Jari Sundell
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

#ifndef RTORRENT_CORE_RANGE_MAP_H
#define RTORRENT_CORE_RANGE_MAP_H

#include <map>
#include <stdexcept>

namespace core {

// Associate values with a range of keys, and retrieve for any key in the range.

// The template arguments have the same semantics as std::map.
// Exception: if set_merge is used, the value type must have a defined operator ==.
template<typename Key, typename T, typename Compare = std::less<Key>,
         typename Alloc = std::allocator<std::pair<const Key, T> > >
class RangeMap : private std::map<Key, std::pair<Key, T>, Compare,
                                 typename Alloc::template rebind<std::pair<const Key, std::pair<Key, T> > >::other> {

  typedef std::map<Key, std::pair<Key, T>, Compare,
                   typename Alloc::template rebind<std::pair<const Key, std::pair<Key, T> > >::other> base_type;

public:
  RangeMap() {}
  RangeMap(const Compare& c) : base_type(c) {}

  typedef typename base_type::iterator iterator;
  typedef typename base_type::reverse_iterator reverse_iterator;
  typedef typename base_type::const_iterator const_iterator;
  typedef typename base_type::const_reverse_iterator const_reverse_iterator;

  // using typename base_type::const_iterator;
  // using typename base_type::const_reverse_iterator;

  using base_type::clear;
  using base_type::swap;

  using base_type::size;
  using base_type::empty;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::key_comp;
  using base_type::value_comp;

  // Store a value for the range [begin, end). Returns iterator for the range.
  const_iterator   set_range(const Key& begin, const Key& end, const T& value);

  // Same, but merge adjacent ranges having the same value. Returns iterator for the merged range.
  const_iterator   set_merge(Key begin, const Key& end, const T& value);

  // Find range containing the given key, or end().
  const_iterator   find(const Key& key) const;

  // Retrieve value for key in a range, throw std::out_of_range if range does not exist.
  const T&         get(const Key& key) const;

  // Retrieve value for key in a range, return def if range does not exist.
  T                get(const Key& key, T def) const;

private:
  iterator         crop_overlap(const Key& begin, const Key& end);
};

// Semantics of an entry:
// .first          End of range (exclusive), map key.
// .second.first   Beginning of range.
// .second.second  Value.

template<typename Key, typename T, typename C, typename A>
inline typename RangeMap<Key,T,C,A>::iterator
RangeMap<Key,T,C,A>::crop_overlap(const Key& _begin, const Key& _end) {
  typename RangeMap::iterator itr = base_type::upper_bound(_begin);

  while (itr != end() && key_comp()(itr->second.first, _end)) {
    // There's a subrange before the new begin: need new entry (new range end means new key).
    if (key_comp()(itr->second.first, _begin))
      base_type::insert(itr, typename RangeMap::value_type(_begin, itr->second));

    // Old end is within our range: erase entry.
    if (!key_comp()(_end, itr->first)) {
      base_type::erase(itr++);

    // Otherwise simply set the new begin of the old range.
    } else {
      itr->second.first = _end;
      ++itr;
    }
  }

  return itr;
}

template<typename Key, typename T, typename C, typename A>
inline typename RangeMap<Key,T,C,A>::const_iterator
RangeMap<Key,T,C,A>::set_merge(Key _begin, const Key& _end, const T& value) {
  if (!key_comp()(_begin, _end))
    return end();

  // Crop overlapping ranges and return iterator to first range after the one we're inserting.
  typename RangeMap::iterator itr = crop_overlap(_begin, _end);

  // Check if range before new one is adjacent and has same value: if so erase it and use its beginning.
  if (itr != begin()) {
    typename RangeMap::iterator prev = itr;
    if (!key_comp()((--prev)->first, _begin) && prev->second.second == value) {
      _begin = prev->second.first;
      base_type::erase(prev);
    }
  }

  // Range after new one is adjacent and has same value: set new beginning.
  if (itr != end() && !key_comp()(_end, itr->second.first) && itr->second.second == value) {
    itr->second.first = _begin;
    return itr;
  }

  // Otherwise, this range isn't mergeable, make new entry.
  return base_type::insert(itr, typename RangeMap::value_type(_end, typename RangeMap::mapped_type(_begin, value)));
}

template<typename Key, typename T, typename C, typename A>
inline typename RangeMap<Key,T,C,A>::const_iterator
RangeMap<Key,T,C,A>::set_range(const Key& _begin, const Key& _end, const T& value) {
  if (!key_comp()(_begin, _end))
    return end();

  return base_type::insert(crop_overlap(_begin, _end), typename RangeMap::value_type(_end, typename RangeMap::mapped_type(_begin, value)));
}

template<typename Key, typename T, typename C, typename A>
inline typename RangeMap<Key,T,C,A>::const_iterator
RangeMap<Key,T,C,A>::find(const Key& key) const {
  typename RangeMap::const_iterator itr = base_type::upper_bound(key);

  if (itr != end() && key_comp()(key, itr->second.first))
    itr = end();

  return itr;
}

template<typename Key, typename T, typename C, typename A>
inline const T&
RangeMap<Key,T,C,A>::get(const Key& key) const {
  typename RangeMap::const_iterator itr = find(key);

  if (itr == end())
    throw std::out_of_range("RangeMap::get");

  return itr->second.second;
}

template<typename Key, typename T, typename C, typename A>
inline T
RangeMap<Key,T,C,A>::get(const Key& key, T def) const {
  typename RangeMap::const_iterator itr = find(key);
  return (itr == end() ? def : itr->second.second);
}

}

#endif
