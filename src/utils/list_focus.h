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

// Can't make this class inherit privately due to gcc PR 14258.

template <typename Base>
class ListFocus {
public:
  typedef typename Base::iterator                   iterator;
  typedef typename Base::const_iterator             const_iterator;
  typedef typename Base::reverse_iterator           reverse_iterator;
  typedef typename Base::const_reverse_iterator     const_reverse_iterator;

  typedef typename Base::value_type                 value_type;
  typedef sigc::signal0<void>                       Signal;

  ListFocus(Base* b = NULL) : m_base(b) { if (b) m_focus = b->end(); }

  // Convinience functions, would have added more through using, but
  // can't.
  iterator            begin()                       { return m_base->begin(); }
  iterator            end()                         { return m_base->end(); }
  reverse_iterator    rbegin()                      { return m_base->rbegin(); }
  reverse_iterator    rend()                        { return m_base->rend(); }

  // Don't do erase on this object without making sure focus is right.
  Base&               base()                        { return *m_base; }

  iterator            get_focus()                   { return m_focus; }
  void                set_focus(iterator itr)       { m_focus = itr; m_signalChanged.emit(); }

  // These are looping increment/decrements.
  iterator            inc_focus();
  iterator            dec_focus();

  iterator            erase(iterator itr);
  void                remove(const value_type& v);

  // Be careful with copying signals.
  Signal&             signal_changed()              { return m_signalChanged; }

private:
  Base*               m_base;
  iterator            m_focus;

  Signal              m_signalChanged;
};

template <typename Base>
typename ListFocus<Base>::iterator
ListFocus<Base>::inc_focus() {
  if (m_focus != end())
    ++m_focus;
  else
    m_focus = begin();

  m_signalChanged.emit();

  return m_focus;
}

template <typename Base>
typename ListFocus<Base>::iterator
ListFocus<Base>::dec_focus() {
  if (m_focus != begin())
    --m_focus;
  else
    m_focus = end();

  m_signalChanged.emit();

  return m_focus;
}

template <typename Base>
typename ListFocus<Base>::iterator
ListFocus<Base>::erase(iterator itr) {
  if (itr == m_focus)
    return m_focus = m_base->erase(itr);
  else
    return m_base->erase(itr);  

  m_signalChanged.emit();
}

template <typename Base>
void
ListFocus<Base>::remove(const value_type& v) {
  iterator first = begin();
  iterator last = end();

  while (first != last)
    if (*first == v)
      first = erase(first);
    else
      ++first;
}

}

#endif
