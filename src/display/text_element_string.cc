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

#include <rak/string_manip.h>

#include "text_element_string.h"

namespace display {

char*
TextElementStringBase::print(char* first, char* last, Canvas::attributes_list* attributes, void* object) {
  Attributes baseAttribute = attributes->back();
  push_attribute(attributes, Attributes(first, m_attributes, Attributes::color_invalid));

  if (first == last)
    return first;

  if (m_flags & flag_escape_hex) {
    char buffer[last - first];
    char* bufferLast = copy_string(buffer, buffer + (last - first), object);

    first = rak::transform_hex(buffer, bufferLast, first, last);

  } else if (m_flags & flag_escape_html) {
    char buffer[last - first];
    char* bufferLast = copy_string(buffer, buffer + (last - first), object);

    first = rak::copy_escape_html(buffer, bufferLast, first, last);

  } else {
    first = copy_string(first, last, object);
  }  

  push_attribute(attributes, Attributes(first, baseAttribute));

  return first;
}

char*
TextElementString::copy_string(char* first, char* last, void* object) {
  extent_type length = std::min<extent_type>(last - first, m_string.size());
  
  std::memcpy(first, m_string.c_str(), length);

  return first + length;
}

char*
TextElementCString::copy_string(char* first, char* last, void* object) {
  extent_type length = std::min<extent_type>(last - first, m_length);

  std::memcpy(first, m_string, length);

  return first + length;
}

}
