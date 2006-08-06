// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

#include "config.h"

#include <torrent/exceptions.h>

#include "display/frame.h"
#include "display/window_text.h"
#include "display/text_element_string.h"
#include "input/manager.h"

#include "control.h"
#include "element_menu.h"

namespace ui {

struct ElementMenuEntry {
  display::TextElementString* m_element;

  ElementMenu::slot_type      m_slotFocus;
  ElementMenu::slot_type      m_slotSelect;
};

ElementMenu::ElementMenu() :
  m_window(new WindowText),
  m_focus(focus_invalid) {

  // Move bindings.
  m_bindings[KEY_UP]    = sigc::mem_fun(this, &ElementMenu::focus_prev);
  m_bindings[KEY_DOWN]  = sigc::mem_fun(this, &ElementMenu::focus_next);
  m_bindings[KEY_RIGHT] = sigc::mem_fun(this, &ElementMenu::focus_select);
}

ElementMenu::~ElementMenu() {
  delete m_window;
}

void
ElementMenu::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::client_error("ui::ElementMenu::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_window->set_active(true);

  m_frame = frame;
  m_frame->initialize_window(m_window);
}

void
ElementMenu::disable() {
  if (!is_active())
    throw torrent::client_error("ui::ElementMenu::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  m_window->set_active(false);
}

void
ElementMenu::push_back(const std::string& name, const slot_type& slotSelect, const slot_type& slotFocus) {
  entry_type* entry = new entry_type;

  entry->m_element    = new display::TextElementString(name);
  entry->m_slotSelect = slotSelect;
  entry->m_slotFocus  = slotFocus;

  m_window->push_back(NULL);
  m_window->push_back(entry->m_element);

  base_type::push_back(entry);

  // For the moment, don't bother doing anything if the window is
  // already active.
  m_window->mark_dirty();
}

void
ElementMenu::focus_next() {
  if (empty() || (size() == 1 && m_focus == 0))
    return;

  if (m_focus < size())
    base_type::operator[](m_focus)->m_element->set_attributes(display::Attributes::a_normal);
  
  if (++m_focus >= size())
    m_focus = 0;

  base_type::operator[](m_focus)->m_slotFocus();
  base_type::operator[](m_focus)->m_element->set_attributes(display::Attributes::a_reverse);

  m_window->mark_dirty();
}

void
ElementMenu::focus_prev() {
  if (empty() || (size() == 1 && m_focus == 0))
    return;

  if (m_focus < size())
    base_type::operator[](m_focus)->m_element->set_attributes(display::Attributes::a_normal);
  
  if (--m_focus >= size())
    m_focus = size() - 1;

  base_type::operator[](m_focus)->m_slotFocus();
  base_type::operator[](m_focus)->m_element->set_attributes(display::Attributes::a_reverse);

  m_window->mark_dirty();
}

void
ElementMenu::focus_select() {
  if (m_focus >= size())
    return;

  base_type::operator[](m_focus)->m_slotSelect();

  m_window->mark_dirty();
}

}
