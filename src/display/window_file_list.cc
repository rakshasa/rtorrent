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
  utils::displayScheduler.insert(&m_taskUpdate, (utils::Timer::cache() + 10 * 1000000).round_seconds());
  m_canvas->erase();

  if (m_download->get_download().get_entry_size() == 0 ||
      m_canvas->get_height() < 2)
    return;

  int pos = 0;

  m_canvas->print( 2, pos, "File");
  m_canvas->print(55, pos, "Size");
  m_canvas->print(63, pos, "Pri");
  m_canvas->print(68, pos, "Cmpl");

  ++pos;

  if (*m_focus >= m_download->get_download().get_entry_size())
    throw std::logic_error("WindowFileList::redraw() called on an object with a bad focus value");

  Range range = rak::advance_bidirectional<unsigned int>(0,
							 *m_focus,
							 m_download->get_download().get_entry_size(),
							 m_canvas->get_height() - pos);

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

    m_canvas->print(0, pos, "%c %s  %6.1f   %s   %3d",
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
