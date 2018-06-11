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

#include <rak/algorithm.h>

#include "core/download.h"
#include "core/view.h"
#include "rpc/parse_commands.h"

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "window_download_list.h"

namespace display {

WindowDownloadList::WindowDownloadList() :
  Window(new Canvas, 0, 120, 1, extent_full, extent_full),
  m_view(NULL) {
}

WindowDownloadList::~WindowDownloadList() {
  if (m_view != NULL)
    m_view->signal_changed().erase(m_changed_itr);
  
  m_view = NULL;
}

void
WindowDownloadList::set_view(core::View* l) {
  if (m_view != NULL)
    m_view->signal_changed().erase(m_changed_itr);

  m_view = l;

  if (m_view != NULL)
    m_changed_itr = m_view->signal_changed().insert(m_view->signal_changed().begin(), std::bind(&Window::mark_dirty, this));
}

void
WindowDownloadList::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());

  m_canvas->erase();

  if (m_view == NULL)
    return;

  m_canvas->print(0, 0, "%s", ("[View: " + m_view->name() + "]").c_str());

  if (m_view->empty_visible() || m_canvas->width() < 5 || m_canvas->height() < 2)
    return;

  // show "X of Y"
  if (m_canvas->width() > 16 + 8 + m_view->name().length()) {
    int item_idx = m_view->focus() - m_view->begin_visible();
    if (item_idx == int(m_view->size()))
      m_canvas->print(m_canvas->width() - 16, 0, "[ none of %-5d]", m_view->size());
    else
      m_canvas->print(m_canvas->width() - 16, 0, "[%5d of %-5d]", item_idx + 1, m_view->size());
  }

  int layout_height;
  const std::string layout_name = rpc::call_command_string("ui.torrent_list.layout");

  if (layout_name == "full") {
    layout_height = 3;
  } else if (layout_name == "compact") {
    layout_height = 1;
  } else {
    m_canvas->print(0, 0, "INVALID ui.torrent_list.layout '%s'", layout_name.c_str());
    return;
  }

  typedef std::pair<core::View::iterator, core::View::iterator> Range;

  Range range = rak::advance_bidirectional(m_view->begin_visible(),
                                           m_view->focus() != m_view->end_visible() ? m_view->focus() : m_view->begin_visible(),
                                           m_view->end_visible(),
                                           m_canvas->height() / layout_height);

  // Make sure we properly fill out the last lines so it looks like
  // there are more torrents, yet don't hide it if we got the last one
  // in focus.
  if (range.second != m_view->end_visible())
    ++range.second;

  int pos = 1;
  char buffer[m_canvas->width() + 1];
  char* last = buffer + m_canvas->width() - 2 + 1;

  // Add a proper 'column info' method.
  if (layout_name == "compact") {
    print_download_column_compact(buffer, last);

    m_canvas->set_default_attributes(A_BOLD);
    m_canvas->print(0, pos++, "  %s", buffer);
  }

  if (layout_name == "full") {
    while (range.first != range.second) {
      print_download_title(buffer, last, *range.first);
      m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);
      print_download_info_full(buffer, last, *range.first);
      m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);
      print_download_status(buffer, last, *range.first);
      m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);

      range.first++;
    }

  } else {
    while (range.first != range.second) {
      print_download_info_compact(buffer, last, *range.first);
      m_canvas->set_default_attributes(range.first == m_view->focus() ? A_REVERSE : A_NORMAL);
      m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);

      range.first++;
    }
  }
}

}
