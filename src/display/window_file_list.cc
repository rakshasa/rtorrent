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

#include <cstdio>
#include <torrent/path.h>
#include <torrent/data/file.h>
#include <torrent/data/file_list.h>
#include <torrent/data/file_list_iterator.h>

#include "core/download.h"
#include "ui/element_file_list.h"

#include "window_file_list.h"

namespace display {

// Don't really like the direction of the element dependency, but
// don't really feel like making a seperate class for containing the
// necessary information.
WindowFileList::WindowFileList(const ui::ElementFileList* element) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_full),
  m_element(element) {
}

/*
std::wstring
hack_wstring(const std::string& src) {
  size_t length = ::mbstowcs(NULL, src.c_str(), src.size());

  if (length == (size_t)-1)
    return std::wstring(L"<invalid>");

  std::wstring dest;
  dest.resize(length);
  
  ::mbstowcs(&*dest.begin(), src.c_str(), src.size());

  return dest;
}
*/

void
WindowFileList::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(10)).round_seconds());
  m_canvas->erase();

  torrent::FileList* fl = m_element->download()->download()->file_list();

  if (fl->size_files() == 0 || m_canvas->height() < 2)
    return;

  iterator entries[m_canvas->height() - 1];

  unsigned int last = 0;

  for (iterator itr = m_element->selected(); last != m_canvas->height() - 1; ) {
    if (m_element->is_collapsed())
      itr.forward_current_depth();
    else
      ++itr;

    entries[last++] = itr;

    if (itr == iterator(fl->end()))
      break;
  }

  unsigned int first = m_canvas->height() - 1;

  for (iterator itr = m_element->selected(); first >= last || first > (m_canvas->height() - 1) / 2; ) {
    entries[--first] = itr;

    if (itr == iterator(fl->begin()))
      break;

    if (m_element->is_collapsed())
      itr.backward_current_depth();
    else
      --itr;
  }

  unsigned int pos = 0;
  m_canvas->print(0, pos++, "Cmp Pri  Size   Filename");

  while (pos != m_canvas->height()) {
    iterator itr = entries[first];

    if (itr == iterator(fl->end()))
      break;

    if (itr.is_empty()) {
      m_canvas->print(16, pos, "EMPTY");

    } else if (itr.is_entering()) {
      m_canvas->print(16 + itr.depth(), pos, "\\ %s", 
                      itr.depth() < (*itr)->path()->size() ? (*itr)->path()->at(itr.depth()).c_str() : "UNKNOWN");

    } else if (itr.is_leaving()) {
      m_canvas->print(16 + itr.depth() - 1, pos, "/");

    } else if (itr.is_file()) {
      char buffer[std::max<unsigned int>(m_canvas->width() + 1, 256)];
      Canvas::attributes_list attributes;

      torrent::File* e = *itr;

      const char* priority;

      switch (e->priority()) {
      case torrent::PRIORITY_OFF:    priority = "off"; break;
      case torrent::PRIORITY_NORMAL: priority = "   "; break;
      case torrent::PRIORITY_HIGH:   priority = "hig"; break;
      default: priority = "BUG"; break;
      };

      sprintf(buffer, "%3d %s ", done_percentage(e), priority);

      int64_t val = e->size_bytes();

      if (val < (int64_t(1000) << 20))
        sprintf(buffer + 8, "%5.1f M", (double)val / (int64_t(1) << 20));
      else if (val < (int64_t(1000) << 30))
        sprintf(buffer + 8, "%5.1f G", (double)val / (int64_t(1) << 30));
      else
        sprintf(buffer + 8, "%5.1f T", (double)val / (int64_t(1) << 40));

      std::fill_n(buffer + 15, 64, ' ');

      int first = 16 + std::min<unsigned int>(itr.depth(), 8);
      int last = std::max<unsigned int>(m_canvas->width() + 1, 16 + 12);

      std::snprintf(buffer + first, last - first, "| %s",
                    itr.depth() < (*itr)->path()->size() ? (*itr)->path()->at(itr.depth()).c_str() : "UNKNOWN");

      m_canvas->print_attributes(0, pos, buffer, buffer + std::strlen(buffer), &attributes);

    } else {
      m_canvas->print(0, pos, "BORK BORK");
    }

    if (itr == m_element->selected())
      m_canvas->set_attr(0, pos, m_canvas->width(), is_focused() ? A_REVERSE : A_BOLD, COLOR_PAIR(0));

    pos++;
    first = (first + 1) % (m_canvas->height() - 1);
  }
}

int
WindowFileList::done_percentage(torrent::File* e) {
  int chunks = e->range().second - e->range().first;

  return chunks ? (e->completed_chunks() * 100) / chunks : 100;
}

}
