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

#include <ctime>

#include "canvas.h"
#include "utils.h"
#include "window_log.h"

namespace display {

WindowLog::WindowLog(core::Log* l) :
  Window(new Canvas, false, 0),
  m_log(l) {

  set_active(false);

  // We're trying out scheduled tasks instead.
  m_connUpdate = l->signal_update().connect(sigc::mem_fun(*this, &WindowLog::receive_update));
}

WindowLog::~WindowLog() {
  m_connUpdate.disconnect();
}

WindowLog::iterator
WindowLog::find_older() {
  return m_log->find_older(utils::Timer::cache() - 60*1000000);
}

void
WindowLog::redraw() {
  m_nextDraw = utils::Timer::max();

  m_canvas->erase();

  int pos = 0;

  //m_canvas->print(std::max(0, (int)m_canvas->get_width() / 2 - 5), pos++, "*** Log ***");
  //m_canvas->print(0, 0, "___");

  for (core::Log::iterator itr = m_log->begin(), end = find_older(); itr != end && pos < m_canvas->get_height(); ++itr)
    m_canvas->print(0, pos++, "(%s) %s",
		    print_hhmmss(itr->first).c_str(),
		    itr->second.c_str());
}

void
WindowLog::receive_update() {
  iterator itr = find_older();

  int h = std::min(std::distance(m_log->begin(), itr),
		   (std::iterator_traits<iterator>::difference_type)10);

  if (h != m_minHeight) {
    set_active(h != 0);

    m_minHeight = h;
    m_slotAdjust();
  }

  mark_dirty();
}

}
