// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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
#include <torrent/exceptions.h>

#include "text_element_list.h"

namespace display {

void
TextElementList::clear() {
  std::for_each(begin(), end(), rak::call_delete<TextElement>());
  base_type::clear();
}

char*
TextElementList::print(char* first, char* last, Canvas::attributes_list* attributes, rpc::target_type target) {
  int column = m_columnWidth != NULL ? m_column : 0;

  // Call print for each element even if first == last so that any
  // attributes gets added to the list.
  for (iterator itr = begin(); itr != end(); ++itr)
    if (column-- > 0) {
      char* columnEnd = std::min(last, first + *m_columnWidth);

      if (columnEnd < first || columnEnd > last)
        throw torrent::internal_error("TextElementList::print(...) columnEnd < first || columnEnd > last.");

      first = (*itr)->print(first, columnEnd, attributes, target);

      if (first > columnEnd)
        throw torrent::internal_error("TextElementList::print(...) first > columnEnd.");

      std::memset(first, ' ', columnEnd - first);
      first = columnEnd;

    } else {
      first = (*itr)->print(first, last, attributes, target);
    }

  return first;
}

TextElementList::extent_type
TextElementList::max_length() {
  extent_type length = 0;
  int column = m_columnWidth != NULL ? m_column : 0;

  for (iterator itr = begin(); itr != end(); ++itr) {
    extent_type l = column-- > 0 ? std::min((*itr)->max_length(), *m_columnWidth) : (*itr)->max_length();

    if (l == extent_full)
      return extent_full;
    
    length += l;
  }
  
  return length;
}

}
