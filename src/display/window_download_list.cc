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

#include <rak/algorithm.h>

#include "core/download.h"

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "window_download_list.h"

namespace display {

WindowDownloadList::WindowDownloadList(DList* l) :
  Window(new Canvas, true),
  m_list(l) {

  m_connChanged = m_list->signal_changed().connect(sigc::mem_fun(*this, &Window::mark_dirty));
}

WindowDownloadList::~WindowDownloadList() {
  m_connChanged.disconnect();
}

void
WindowDownloadList::redraw() {
  m_slotSchedule(this, (cachedTime + 1000000).round_seconds());

  m_canvas->erase();

  if (m_list->base().empty() || m_canvas->get_width() < 5)
    return;

  typedef std::pair<DList::iterator, DList::iterator> Range;

  Range range = rak::advance_bidirectional(m_list->begin(),
					   m_list->get_focus() != m_list->end() ? m_list->get_focus() : m_list->begin(),
					   m_list->end(),
					   m_canvas->get_height() / 3);

  // Make sure we properly fill out the last lines so it looks like
  // there are more torrents, yet don't hide it if we got the last one
  // in focus.
  if (range.second != m_list->end())
    ++range.second;

  int pos = 0;

  while (range.first != range.second) {
    char buffer[m_canvas->get_width() - 2];
    char* position;
    char* last = buffer + m_canvas->get_width() - 2;

    position = print_download_title(buffer, last - buffer, *range.first);
    m_canvas->print(0, pos++, "%c %s", range.first == m_list->get_focus() ? '*' : ' ', buffer);
    
    position = print_download_info(buffer, last - buffer, *range.first);
    m_canvas->print(0, pos++, "%c %s", range.first == m_list->get_focus() ? '*' : ' ', buffer);

    position = print_download_status(buffer, last - buffer, *range.first);
    m_canvas->print(0, pos++, "%c %s", range.first == m_list->get_focus() ? '*' : ' ', buffer);

    ++range.first;
  }    
}

}
