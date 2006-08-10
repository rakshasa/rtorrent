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

#ifndef RTORRENT_UI_ELEMENT_TEXT_H
#define RTORRENT_UI_ELEMENT_TEXT_H

#include <sigc++/slot.h>

#include "core/download.h"
#include "display/text_element_string.h"

#include "element_base.h"

namespace display {
  class WindowText;
  class TextElement;
}

namespace ui {

struct text_element_wrapper;

class ElementText : public ElementBase {
public:
  typedef display::WindowText         WindowText;

  typedef uint32_t                    size_type;
  typedef uint32_t                    extent_type;

  ElementText(void *object);
  ~ElementText();

  void                activate(display::Frame* frame, bool focus = false);
  void                disable();

  // Consider returning a pointer that can be used to manipulate
  // entries, f.ex disabling them.

  void                push_back(text_element_wrapper entry);

  void                push_column(text_element_wrapper entry1, text_element_wrapper entry2);

  void                set_column(unsigned int column)     { m_column = column; }

  extent_type         column_width() const                { return m_columnWidth; }
  void                set_column_width(extent_type width) { m_columnWidth = width; }

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
