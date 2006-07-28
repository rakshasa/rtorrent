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

#include <stdexcept>
#include <rak/algorithm.h>
#include <rak/string_manip.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>

#include "core/download.h"

#include "window_tracker_list.h"

namespace display {

WindowTrackerList::WindowTrackerList(core::Download* d, unsigned int* focus) :
  Window(new Canvas, flag_width_dynamic | flag_height_dynamic, 0, 0),
  m_download(d),
  m_focus(focus) {
}

void
WindowTrackerList::redraw() {
  // TODO: Make this depend on tracker signal.
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(10)).round_seconds());
  m_canvas->erase();

  int pos = 0;
  torrent::TrackerList* tl = m_download->tracker_list();

  m_canvas->print(2, pos, "Trackers: [Key: %08x]", tl->key());
  ++pos;

  if (tl->size() == 0)
    return;

  if (*m_focus >= tl->size())
    throw std::logic_error("WindowTrackerList::redraw() called on an object with a bad focus value.");

  typedef std::pair<unsigned int, unsigned int> Range;

  Range range = rak::advance_bidirectional<unsigned int>(0, *m_focus, tl->size(), (m_canvas->height() + 1) / 2);

  while (range.first != range.second) {
    torrent::Tracker t = tl->get(range.first);

    m_canvas->print(0, pos++, "%c %s",
                    range.first == *m_focus ? '*' : ' ',
                    t.url().c_str());

    m_canvas->print(0, pos++, "%c Group: %2i Id: %s Focus: %s Enabled: %s Open: %s S/L: %u/%u",
                    range.first == *m_focus ? '*' : ' ',
                    t.group(),
                    rak::copy_escape_html(t.tracker_id()).c_str(),
                    range.first == tl->focus() ? "yes" : " no",
                    t.is_enabled() ? "yes" : " no",
                    t.is_open() ? "yes" : " no",
                    t.scrape_complete(),
                    t.scrape_incomplete());

    ++range.first;
  }
}

}
