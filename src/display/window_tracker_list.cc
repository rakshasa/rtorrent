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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <stdexcept>

#include "core/download.h"
#include "rak/algorithm.h"
#include "utils/parse.h"

#include "window_tracker_list.h"

namespace display {

WindowTrackerList::WindowTrackerList(core::Download* d, unsigned int* focus) :
  Window(new Canvas, true),
  m_download(d),
  m_focus(focus) {
}

void
WindowTrackerList::redraw() {
  // TODO: Make this depend on tracker signal.
  m_nextDraw = utils::Timer::cache().round_seconds() + 10 * 1000000;
  m_canvas->erase();

  int pos = 0;

  m_canvas->print( 2, pos, "Trackers:");

  ++pos;

  if (m_download->get_download().get_tracker_size() == 0)
    return;

  if (*m_focus >= m_download->get_download().get_tracker_size())
    throw std::logic_error("WindowTrackerList::redraw() called on an object with a bad focus value");

  typedef std::pair<unsigned int, unsigned int> Range;

  Range range = rak::advance_bidirectional<unsigned int>(0,
							 *m_focus,
							 m_download->get_download().get_tracker_size(),
							 (m_canvas->get_height() + 1) / 2);

  while (range.first != range.second) {
    torrent::Tracker t = m_download->get_download().get_tracker(range.first);

    m_canvas->print(0, pos++, "%c %s",
		    range.first == *m_focus ? '*' : ' ',
		    t.get_url().c_str());

    m_canvas->print(0, pos++, "%c Group: %2i Id: %s",
		    range.first == *m_focus ? '*' : ' ',
		    t.get_group(),
		    utils::escape_string(t.get_tracker_id()).c_str());

    ++range.first;
  }
}

}
