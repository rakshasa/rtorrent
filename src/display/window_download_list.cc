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
#include <torrent/rate.h>

#include "core/download.h"
#include "core/view.h"

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

  typedef std::pair<core::View::iterator, core::View::iterator> Range;

  Range range = rak::advance_bidirectional(m_view->begin_visible(),
                                           m_view->focus() != m_view->end_visible() ? m_view->focus() : m_view->begin_visible(),
                                           m_view->end_visible(),
                                           m_canvas->height() / 3);

  // Make sure we properly fill out the last lines so it looks like
  // there are more torrents, yet don't hide it if we got the last one
  // in focus.
  if (range.second != m_view->end_visible())
    ++range.second;

  int pos = 1;

  while (range.first != range.second) {
    char buffer[m_canvas->width() + 1];
    char* last = buffer + m_canvas->width() - 2 + 1;

    /*
    There are four states where colors are applied:
    1 inactive  # If inactive, e.g. user stopped down/upload
    2 dead      # If active but no down/upload
    3 active    # If active and download
    4 finished  # If finished
    */

    print_download_title(buffer, last, *range.first);
    
    m_canvas->print(0, pos, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);
    if( (*range.first)->is_done() ) {
      if( (*range.first)->download()->info()->up_rate()->rate() != 0 ) {
      m_canvas->set_attr(0, pos, m_canvas->width()-1, A_BOLD, 4);       // Finished and uploading
      } else {
     m_canvas->set_attr(0, pos, m_canvas->width()-1, A_NORMAL, 4);     // Finished
      }
    } else if( (*range.first)->is_active() ) {
      if( (*range.first)->download()->info()->down_rate()->rate() != 0 ) {
        if( (*range.first)->download()->info()->up_rate()->rate() != 0 ) {
          m_canvas->set_attr(0, pos, m_canvas->width()-1, A_BOLD, 3);     // Active and uploading
        } else {
          m_canvas->set_attr(0, pos, m_canvas->width()-1, A_NORMAL, 3);   // Active
        }
      } else {
        if( (*range.first)->download()->info()->up_rate()->rate() != 0 ) {
          m_canvas->set_attr(0, pos, m_canvas->width()-1, A_BOLD, 2);     // Dead but still uploading
        } else {
          m_canvas->set_attr(0, pos, m_canvas->width()-1, A_NORMAL, 2);   // Dead
        }
      }
    } else {
      m_canvas->set_attr(0, pos, m_canvas->width()-1, A_NORMAL, 1);     // Inactive
    }
    pos++;
    
    print_download_info(buffer, last, *range.first);
    m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);

    print_download_status(buffer, last, *range.first);
    m_canvas->print(0, pos++, "%c %s", range.first == m_view->focus() ? '*' : ' ', buffer);

    ++range.first;
  }    
}

// Inactive
void
WindowDownloadList::set_color_inactive_fg(int64_t color) {
  short fg, bg;
  pair_content(1, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(1, (short)color, bg);
}

void
WindowDownloadList::set_color_inactive_bg(int64_t color) {
  short fg, bg;
  pair_content(1, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(1, fg, (short)color);
}

// Dead
void
WindowDownloadList::set_color_dead_fg(int64_t color) {
  short fg, bg;
  pair_content(2, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(2, (short)color, bg);
}

void
WindowDownloadList::set_color_dead_bg(int64_t color) {
  short fg, bg;
  pair_content(2, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(2, fg, (short)color);
}

// Active
void
WindowDownloadList::set_color_active_fg(int64_t color) {
  short fg, bg;
  pair_content(3, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(3, (short)color, bg);
}

void
WindowDownloadList::set_color_active_bg(int64_t color) {
  short fg, bg;
  pair_content(3, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(3, fg, (short)color);
}

// Finished
void
WindowDownloadList::set_color_finished_fg(int64_t color) {
  short fg, bg;
  pair_content(4, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(4, (short)color, bg);
}

void
WindowDownloadList::set_color_finished_bg(int64_t color) {
  short fg, bg;
  pair_content(4, &fg, &bg);
  if( color < 0 ) color = -1;
  color = color % 8;
  init_pair(4, fg, (short)color);
}
}
