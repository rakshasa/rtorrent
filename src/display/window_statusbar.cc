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

#include <torrent/torrent.h>

#include "core/manager.h"

#include "canvas.h"
#include "window_statusbar.h"

namespace display {

WindowStatusbar::WindowStatusbar(core::Manager* c) :
  Window(new Canvas, false, 1),
  m_counter(0),
  m_core(c) {
}

void
WindowStatusbar::redraw() {
  m_nextDraw = utils::Timer::cache().round_seconds() + 1000000;

  m_canvas->erase();
  m_canvas->print(0, 0, "Throttle: %i Listen: %s:%i Handshakes: %i",
		  (int)torrent::get(torrent::THROTTLE_ROOT_CONST_RATE) / 1024,
		  m_core->get_dns().empty() ? "<default>" : m_core->get_dns().c_str(),
		  (int)torrent::get(torrent::LISTEN_PORT),
		  (int)torrent::get(torrent::HANDSHAKES_TOTAL));
}

}
