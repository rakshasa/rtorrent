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

#include <locale>
#include <stdio.h>
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

// Convert std::string to std::wstring of given width (in screen positions),
// taking into account that some characters may be occupying two screen positions.
std::wstring
wstring_width(const std::string& i_str, int width) {
  wchar_t result[width + 1];
  size_t length = std::mbstowcs(result, i_str.c_str(), width);

  // If not valid UTF-8 encoding, at least copy the printable characters.
  if (length == (size_t)-1) {
    wchar_t* out = result;

    for (std::string::const_iterator itr = i_str.begin(); out != result + width && itr != i_str.end(); ++itr)
      if (!std::isprint(*itr, std::locale::classic()))
        *out++ = '?';
      else
        *out++ = *itr;

     *out = 0;
  }

  int swidth = wcswidth(result, width);

  // Limit to width if it's too wide already.
  if (swidth == -1 || swidth > width) {
    length = swidth = 0;
    
    while (result[length]) {
      int next = ::wcwidth(result[length]);

      // Unprintable character?
      if (next == -1) {
        result[length] = '?';
        next = 1;
      }

      if (swidth + next > width) {
        result[length] = 0;
        break;
      }

      length++;
      swidth += next;
    }
  }

  // Pad with spaces to given width.
  while (swidth < width && length <= (unsigned int)width) {
    result[length++] = ' ';
    swidth++;
  }

  result[length] = 0;

  return result;
}

void
WindowFileList::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(10)).round_seconds());
  m_canvas->erase();

  torrent::FileList* fl = m_element->download()->download()->file_list();

  if (fl->size_files() == 0 || m_canvas->height() < 2)
    return;

  std::vector<iterator> entries(m_canvas->height() - 1);

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
  int filenameWidth = m_canvas->width() - 16;

  m_canvas->print(0, pos++, "Cmp Pri  Size   Filename");

  while (pos != m_canvas->height()) {
    iterator itr = entries[first];

    if (itr == iterator(fl->end()))
      break;

    m_canvas->set_default_attributes(itr == m_element->selected() ? is_focused() ? A_REVERSE : A_BOLD : A_NORMAL);

    if (itr.is_empty()) {
      m_canvas->print(0, pos, "%*c%-*s", 16, ' ', filenameWidth, "EMPTY");

    } else if (itr.is_entering()) {
      m_canvas->print(0, pos, "%*c %ls", 16 + itr.depth(), '\\',
                      itr.depth() < (*itr)->path()->size() ? wstring_width((*itr)->path()->at(itr.depth()), filenameWidth - itr.depth() - 1).c_str() : L"UNKNOWN");

    } else if (itr.is_leaving()) {
      m_canvas->print(0, pos, "%*c %-*s", 16 + (itr.depth() - 1), '/', filenameWidth - (itr.depth() - 1), "");

    } else if (itr.is_file()) {
      torrent::File* e = *itr;

      const char* priority;

      switch (e->priority()) {
      case torrent::PRIORITY_OFF:    priority = "off"; break;
      case torrent::PRIORITY_NORMAL: priority = "   "; break;
      case torrent::PRIORITY_HIGH:   priority = "hig"; break;
      default: priority = "BUG"; break;
      };

      m_canvas->print(0, pos, "%3d %s ", done_percentage(e), priority);

      int64_t val = e->size_bytes();

      if (val < (int64_t(1000) << 10))
        m_canvas->print(8, pos, "%5.1f K", (double)val / (int64_t(1) << 10));
      else if (val < (int64_t(1000) << 20))
        m_canvas->print(8, pos, "%5.1f M", (double)val / (int64_t(1) << 20));
      else if (val < (int64_t(1000) << 30))
        m_canvas->print(8, pos, "%5.1f G", (double)val / (int64_t(1) << 30));
      else
        m_canvas->print(8, pos, "%5.1f T", (double)val / (int64_t(1) << 40));

      m_canvas->print(15, pos, "%*c %ls", 1 + itr.depth(), '|',
                      itr.depth() < (*itr)->path()->size() ? wstring_width((*itr)->path()->at(itr.depth()), filenameWidth - itr.depth() - 1).c_str() : L"UNKNOWN");

    } else {
      m_canvas->print(0, pos, "BORK BORK");
    }
    m_canvas->set_default_attributes(A_NORMAL);

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
