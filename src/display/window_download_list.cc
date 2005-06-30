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

#include "core/download.h"
#include "rak/algorithm.h"

#include "canvas.h"
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
  utils::displayScheduler.insert(&m_taskUpdate, utils::Timer::cache().round_seconds() + 1000000);

  m_canvas->erase();

  if (m_list->base().empty())
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
    torrent::Download& d = (*range.first)->get_download();

    m_canvas->print(0, pos++, "%c %s",
		    range.first == m_list->get_focus() ? '*' : ' ',
		    d.get_name().c_str());

    if ((*range.first)->is_open() && (*range.first)->is_done())
      m_canvas->print(0, pos++, "%c Torrent: Done %10.1f MiB Rate: %5.1f / %5.1f KiB Uploaded: %.1f MiB",
		      range.first == m_list->get_focus() ? '*' : ' ',
		      (double)d.get_bytes_total() / (double)(1 << 20),
		      (double)d.get_rate_up() / 1024.0,
		      (double)d.get_rate_down() / 1024.0,
		      (double)d.get_bytes_up() / (double)(1 << 20));
    else
      m_canvas->print(0, pos++, "%c Torrent: %6.1f / %6.1f MiB Rate: %5.1f / %5.1f KiB Uploaded: %.1f MiB",
		      range.first == m_list->get_focus() ? '*' : ' ',
		      (double)d.get_bytes_done() / (double)(1 << 20),
		      (double)d.get_bytes_total() / (double)(1 << 20),
		      (double)d.get_rate_up() / 1024.0,
		      (double)d.get_rate_down() / 1024.0,
		      (double)d.get_bytes_up() / (double)(1 << 20));
    
    m_canvas->print(0, pos++, "%c %s",
		    range.first == m_list->get_focus() ? '*' : ' ',
		    print_download_status(*range.first).c_str());

    ++range.first;
  }    
}

}
