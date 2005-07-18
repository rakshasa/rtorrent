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

#include <torrent/rate.h>
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
  utils::displayScheduler.insert(&m_taskUpdate, utils::Timer::cache().round_seconds() + 1000000);

  m_canvas->erase();

  // TODO: Make a buffer with size = get_width?
  int pos = 0;
  char buf[128];

  if (torrent::get_write_throttle() == 0)
    pos = snprintf(buf, 128, "off/");
  else
    pos = snprintf(buf, 128, "%3i/", torrent::get_write_throttle() / 1024);

  if (torrent::get_read_throttle() == 0)
    pos = snprintf(buf + pos, 128 - pos, "off");
  else
    pos = snprintf(buf + pos, 128 - pos, "%-3i", torrent::get_read_throttle() / 1024);

  m_canvas->print(0, 0, "Throttle U/D: %s  Rate: %5.1f / %5.1f KiB  Listen: %s:%i%s",
		  buf,
		  (double)torrent::get_write_rate().rate() / 1024.0,
		  (double)torrent::get_read_rate().rate() / 1024.0,
		  !torrent::get_address().empty() ? torrent::get_address().c_str() : "<default>",
		  (int)torrent::get_listen_port(),
		  !torrent::get_bind_address().empty() ? ("  Bind: " + torrent::get_bind_address()).c_str() : "");

  pos = snprintf(buf, 128, "[%3i/%3i/%3i]",
		 torrent::get_total_handshakes(),
		 torrent::get_open_sockets(),
		 torrent::get_max_open_sockets());

  m_canvas->print(m_canvas->get_width() - pos, 0, "%s", buf);
}

}
