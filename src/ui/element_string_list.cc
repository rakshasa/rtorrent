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

#include "config.h"

#include <stdexcept>

#include "control.h"
#include "element_string_list.h"

namespace ui {

ElementStringList::ElementStringList() {
  m_list.push_back("Test string 1");
  m_list.push_back("Test string 2");
}

void
ElementStringList::activate(Control* c, MItr mItr) {
  if (m_window != NULL)
    throw std::logic_error("ui::ElementStringList::activate(...) called on an object in the wrong state");

  c->get_input().push_front(&m_bindings);

  *mItr = m_window = new WStringList();

  m_window->set_range(m_list.begin(), m_list.end());
}

void
ElementStringList::disable(Control* c) {
  if (m_window == NULL)
    throw std::logic_error("ui::ElementStringList::disable(...) called on an object in the wrong state");

  c->get_input().erase(&m_bindings);

  delete m_window;
  m_window = NULL;
}

void
ElementStringList::next_screen() {
  if (m_window->get_draw_end() != m_list.end())
    m_window->set_range(m_window->get_draw_end(), m_list.end());
  else
    m_window->set_range(m_list.begin(), m_list.end());

  m_window->mark_dirty();
}

}
