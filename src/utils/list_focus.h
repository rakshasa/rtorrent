// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#ifndef RTORRENT_UTILS_LIST_FOCUS_H
#define RTORRENT_UTILS_LIST_FOCUS_H

#include <tr1/functional>

namespace utils {

// Can't make this class inherit privately due to gcc PR 14258.

template <typename Base>
class ListFocus {
public:
  typedef Base                        base_type;
  typedef std::tr1::function<void ()> slot_void;
  typedef std::list<slot_void>        signal_void;

  typedef typename base_type::iterator               iterator;
  typedef typename base_type::const_iterator         const_iterator;
  typedef typename base_type::reverse_iterator       reverse_iterator;
  typedef typename base_type::const_reverse_iterator const_reverse_iterator;

  typedef typename base_type::value_type             value_type;

  ListFocus(base_type* b = NULL) : m_base(b) { if (b) m_focus = b->end(); }

  // Convinience functions, would have added more through using, but
  // can't.
  iterator            begin()                       { return m_base->begin(); }
  iterator            end()                         { return m_base->end(); }
  reverse_iterator    rbegin()                      { return m_base->rbegin(); }
  reverse_iterator    rend()                        { return m_base->rend(); }

  // Don't do erase on this object without making sure focus is right.
  base_type&          base()                        { return *m_base; }

  iterator            get_focus()                   { return m_focus; }
  void                set_focus(iterator itr);

  // These are looping increment/decrements.
  iterator            inc_focus();
  iterator            dec_focus();

  iterator            erase(iterator itr);
  void                remove(const value_type& v);

  // Be careful with copying signals.
  signal_void&        signal_changed()              { return m_signal_changed; }

private:
  void                emit_changed();

  base_type*          m_base;
  iterator            m_focus;

  signal_void         m_signal_changed;
};

template <typename Base>
void
ListFocus<Base>::set_focus(iterator itr) {
  m_focus = itr;
  emit_changed();
}

template <typename Base>
typename ListFocus<Base>::iterator
ListFocus<Base>::inc_focus() {
  if (m_focus != end())
    ++m_focus;
  else
    m_focus = begin();

  emit_changed();
  return m_focus;
}

template <typename Base>
typename ListFocus<Base>::iterator
ListFocus<Base>::dec_focus() {
  if (m_focus != begin())
    --m_focus;
  else
    m_focus = end();

  emit_changed();
  return m_focus;
}

template <typename Base>
typename ListFocus<Base>::iterator
ListFocus<Base>::erase(iterator itr) {
  if (itr == m_focus)
    return m_focus = m_base->erase(itr);
  else
    return m_base->erase(itr);  

  emit_changed();
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

template <typename Base>
void
ListFocus<Base>::emit_changed() {
  for (signal_void::iterator itr = m_signal_changed.begin(), last = m_signal_changed.end(); itr != last; itr++)
    (*itr)();
}

}

#endif
