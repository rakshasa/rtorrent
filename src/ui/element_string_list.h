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

#ifndef RTORRENT_UI_ELEMENT_STRING_LIST_H
#define RTORRENT_UI_ELEMENT_STRING_LIST_H

#include <list>
#include <string>
#include <algorithm>
#include <functional>

#include "element_base.h"

#include "display/window_string_list.h"

namespace ui {

class Control;

class ElementStringList : public ElementBase {
public:
  typedef display::WindowStringList     WStringList;
  typedef std::list<std::string>        List;

  ElementStringList();

  void                activate(Control* c, MItr mItr);
  void                disable(Control* c);

  template <typename InputIter>
  void                set_range(InputIter first, InputIter last) {
    m_list.clear();

    while (first != last)
      m_list.push_back(*(first++));

    m_window->set_range(m_list.begin(), m_list.end());
    m_window->mark_dirty();
  }

private:
  WStringList*        m_window;
  List                m_list;
};

}

#endif
