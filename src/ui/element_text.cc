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

#include "config.h"

#include <torrent/exceptions.h>

#include "display/frame.h"
#include "display/window_text.h"
#include "display/text_element_list.h"
#include "input/manager.h"

#include "control.h"
#include "element_text.h"

namespace ui {

ElementText::ElementText(rpc::target_type target) :
  m_window(new WindowText(target)),
  m_column(0),
  m_columnWidth(0) {

  // Move bindings into a function that defines default bindings.
  m_bindings[KEY_LEFT] = m_bindings['B' - '@'] = std::tr1::bind(&slot_type::operator(), &m_slot_exit);

//   m_bindings[KEY_UP]    = std::tr1::bind(this, &ElementText::entry_prev);
//   m_bindings[KEY_DOWN]  = std::tr1::bind(this, &ElementText::entry_next);
//   m_bindings[KEY_RIGHT] = m_bindings['F' - '@'] = std::tr1::bind(this, &ElementText::entry_select);
}

ElementText::~ElementText() {
  delete m_window;
}

void
ElementText::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::ElementText::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_focus = focus;

  m_frame = frame;
  m_frame->initialize_window(m_window);

  m_window->set_active(true);
}

void
ElementText::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::ElementText::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  m_window->set_active(false);
}

void
ElementText::push_back(text_element_wrapper entry) {
  m_window->push_back(entry.m_element);

  // For the moment, don't bother doing anything if the window is
  // already active.
  m_window->mark_dirty();
}

void
ElementText::push_column(text_element_wrapper entry1, text_element_wrapper entry2) {
  m_columnWidth = std::max(entry1.m_element->max_length(), m_columnWidth);

  display::TextElementList* list = new display::TextElementList;
  list->set_column(m_column);
  list->set_column_width(&m_columnWidth);

  list->push_back(entry1.m_element);
  list->push_back(entry2.m_element);

  push_back(list);
}

void
ElementText::push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                         text_element_wrapper entry3) {
  m_columnWidth = std::max(entry1.m_element->max_length(), m_columnWidth);

  display::TextElementList* list = new display::TextElementList;
  list->set_column(m_column);
  list->set_column_width(&m_columnWidth);

  list->push_back(entry1.m_element);
  list->push_back(entry2.m_element);
  list->push_back(entry3.m_element);

  push_back(list);
}

void
ElementText::push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                         text_element_wrapper entry3, text_element_wrapper entry4) {
  m_columnWidth = std::max(entry1.m_element->max_length(), m_columnWidth);

  display::TextElementList* list = new display::TextElementList;
  list->set_column(m_column);
  list->set_column_width(&m_columnWidth);

  list->push_back(entry1.m_element);
  list->push_back(entry2.m_element);
  list->push_back(entry3.m_element);
  list->push_back(entry4.m_element);

  push_back(list);
}

void
ElementText::push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                         text_element_wrapper entry3, text_element_wrapper entry4,
                         text_element_wrapper entry5) {
  m_columnWidth = std::max(entry1.m_element->max_length(), m_columnWidth);

  display::TextElementList* list = new display::TextElementList;
  list->set_column(m_column);
  list->set_column_width(&m_columnWidth);

  list->push_back(entry1.m_element);
  list->push_back(entry2.m_element);
  list->push_back(entry3.m_element);
  list->push_back(entry4.m_element);
  list->push_back(entry5.m_element);

  push_back(list);
}

void
ElementText::push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                         text_element_wrapper entry3, text_element_wrapper entry4,
                         text_element_wrapper entry5, text_element_wrapper entry6) {
  m_columnWidth = std::max(entry1.m_element->max_length(), m_columnWidth);

  display::TextElementList* list = new display::TextElementList;
  list->set_column(m_column);
  list->set_column_width(&m_columnWidth);

  list->push_back(entry1.m_element);
  list->push_back(entry2.m_element);
  list->push_back(entry3.m_element);
  list->push_back(entry4.m_element);
  list->push_back(entry5.m_element);
  list->push_back(entry6.m_element);

  push_back(list);
}

}
