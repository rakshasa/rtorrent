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

#include <algorithm>
#include <rak/functional.h>

#include "canvas.h"
#include "utils.h"
#include "window_text.h"

namespace display {

void
WindowText::clear() {
  std::for_each(begin(), end(), rak::call_delete<TextElement>());
  base_type::clear();
}

void
WindowText::push_back(TextElement* element) {
  base_type::push_back(element);

//   m_minHeight = size();
  m_maxHeight = size();

  if (element != NULL)
    m_maxWidth = std::max(m_maxWidth, element->max_length() + 2);

  // Check if active, if so do the update thingie. Or be lazy?
}

void
WindowText::redraw() {
  m_canvas->erase();

  unsigned int position = 0;

  for (iterator itr = begin(); itr != end() && position < m_canvas->height(); ++itr, ++position) {
    if (*itr == NULL)
      continue;

    char buffer[m_canvas->width() + 1];

    // Add a print function that sets up attributes etc?
    Canvas::attributes_list attributes;
    attributes.push_back(Attributes(buffer, Attributes::a_normal, Attributes::color_default));

    char* last = (*itr)->print(buffer, buffer + m_canvas->width(), &attributes, NULL);

    m_canvas->print_attributes(0, position, buffer, last, &attributes);
  }
}

}
