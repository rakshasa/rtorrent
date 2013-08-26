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

#ifndef RTORRENT_UI_ELEMENT_TEXT_H
#define RTORRENT_UI_ELEMENT_TEXT_H

#include "core/download.h"
#include "display/text_element_string.h"
#include "display/window_text.h"

#include "element_base.h"

namespace display {
  class TextElement;
}

namespace ui {

struct text_element_wrapper;

class ElementText : public ElementBase {
public:
  typedef display::WindowText         WindowText;

  typedef uint32_t                    size_type;
  typedef uint32_t                    extent_type;

  ElementText(rpc::target_type target);
  ~ElementText();

  rpc::target_type    target() const                      { return m_window->target(); }
  void                set_target(rpc::target_type target) { m_window->set_target(target); m_window->mark_dirty(); }

  uint32_t            interval() const         { return m_window->interval(); }
  void                set_interval(uint32_t i) { m_window->set_interval(i); m_window->mark_dirty(); }

  void                activate(display::Frame* frame, bool focus = false);
  void                disable();

  void                mark_dirty()             { m_window->mark_dirty(); }

  // Consider returning a pointer that can be used to manipulate
  // entries, f.ex disabling them.

  void                push_back(text_element_wrapper entry);

  void                push_column(text_element_wrapper entry1, text_element_wrapper entry2);

  void                push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                                  text_element_wrapper entry3);

  void                push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                                  text_element_wrapper entry3, text_element_wrapper entry4);

  void                push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                                  text_element_wrapper entry3, text_element_wrapper entry4,
                                  text_element_wrapper entry5);

  void                push_column(text_element_wrapper entry1, text_element_wrapper entry2,
                                  text_element_wrapper entry3, text_element_wrapper entry4,
                                  text_element_wrapper entry5, text_element_wrapper entry6);

  void                set_column(unsigned int column)            { m_column = column; }
  void                set_error_handler(display::TextElement* t) { m_window->set_error_handler(t); }

  extent_type         column_width() const                       { return m_columnWidth; }
  void                set_column_width(extent_type width)        { m_columnWidth = width; }

private:
  WindowText*         m_window;

  unsigned int        m_column;
  extent_type         m_columnWidth;
};

struct text_element_wrapper {
  text_element_wrapper(const char* cstr) : m_element(new display::TextElementCString(cstr)) {}
  text_element_wrapper(display::TextElement* element) : m_element(element) {}

  display::TextElement* m_element;
};  

}

#endif
