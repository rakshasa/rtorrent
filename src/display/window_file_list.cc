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

#include "window_file_list.h"

namespace display {

WindowFileList::WindowFileList(core::Download* d, unsigned int* focus) :
  Window(new Canvas, true),
  m_download(d),
  m_focus(focus) {
}

void
WindowFileList::redraw() {
  m_nextDraw = utils::Timer::cache().round_seconds() + 10 * 1000000;
  m_canvas->erase();

  int pos = 0;

  m_canvas->print( 2, pos, "File");
  m_canvas->print(55, pos, "Size");
  m_canvas->print(62, pos, "Pri");
  m_canvas->print(67, pos, "Cmpl");

  ++pos;

  if (m_download->get_download().get_entry_size() == 0)
    return;

  if (*m_focus >= m_download->get_download().get_entry_size())
    throw std::logic_error("WindowFileList::redraw() called on an object with a bad focus value");

  typedef std::pair<unsigned int, unsigned int> Range;

  Range range = rak::advance_bidirectional<unsigned int>(0,
							 *m_focus,
							 m_download->get_download().get_entry_size(),
							 m_canvas->get_height());

  while (range.first != range.second) {
    torrent::Entry e = m_download->get_download().get_entry(range.first);

    std::string path = e.get_path();

    if (path.length() <= 50)
      path = path + std::string(50 - path.length(), ' ');
    else
      path = path.substr(0, 50);

    std::string priority;

    switch (e.get_priority()) {
    case torrent::Entry::STOPPED:
      priority = "off";
      break;

    case torrent::Entry::NORMAL:
      priority = "   ";
      break;

    case torrent::Entry::HIGH:
      priority = "hig";
      break;

    default:
      priority = "BUG";
      break;
    };

    m_canvas->print(0, pos, "%c %s  %5.1f   %s   %3d",
		    range.first == *m_focus ? '*' : ' ',
		    path.c_str(),
		    (double)e.get_size() / (double)(1 << 20),
		    priority.c_str(),
		    done_percentage(e));

    ++range.first;
    ++pos;
  }

}

int
WindowFileList::done_percentage(torrent::Entry& e) {
  int chunks = e.get_chunk_end() - e.get_chunk_begin();

  return chunks ? (e.get_completed() * 100) / chunks : 100;
}

}
