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

#ifndef RTORRENT_UTILS_LIST_FOCUS_H
#define RTORRENT_UTILS_LIST_FOCUS_H

#include <sigc++/signal.h>

namespace utils {

template <typename Base>
class ListFocus : private Base {
public:
  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::value_type;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::insert;
  using Base::push_back;
  using Base::push_front;

  ListFocus() : m_focus(end()) {}

  // Don't do erase on this object.
  Base&               get_list()  { return m_list; }
  typename iterator&  get_focus() { return m_focus; }

  typename iterator   erase(typename iterator itr);
  void                remove(typename const value_type& v);

  // Be careful with copying signals.

private:
  typename iterator   m_focus;
};

template <typename Base>
typename ListFocus<Base>::iterator
ListFocus<Base>::erase(typename iterator itr) {
  if (itr == m_focus)
    return m_focus = Base::erase(itr);
  else
    return Base::erase(itr);  
}

template <typename Base>
void
ListFocus<Base>::remove(typename const value_type& v) {
  typename iterator first = begin();
  typename iterator last = end();

  while (first != last)
    if (*first == v)
      first = erase(first);
    else
      ++first;
}

}

#endif
