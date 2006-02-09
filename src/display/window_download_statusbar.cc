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

#include <torrent/rate.h>

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "window_download_statusbar.h"

#include "core/download.h"

namespace display {

WindowDownloadStatusbar::WindowDownloadStatusbar(core::Download* d) :
  Window(new Canvas, false, 3),
  m_download(d) {
}

void
WindowDownloadStatusbar::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());

  m_canvas->erase();

  char buffer[m_canvas->get_width()];
  char* position;
  char* last = buffer + m_canvas->get_width() - 2;

  position = print_download_info(buffer, last, m_download);
  m_canvas->print(0, 0, "%s", buffer);

  position = buffer + std::max(snprintf(buffer, last - buffer, "Peers: %i(%i) Min/Max: %i/%i Uploads: %i U/I: %i/%i Failed: %i",
					(int)m_download->get_download().peers_connected(),
					(int)m_download->get_download().peers_not_connected(),
					(int)m_download->get_download().peers_min(),
					(int)m_download->get_download().peers_max(),
					(int)m_download->get_download().uploads_max(),
					(int)m_download->get_download().peers_currently_unchoked(),
					(int)m_download->get_download().peers_currently_interested(),
					(int)m_download->chunks_failed()),
			       0);
//   position = buffer + std::max(snprintf(position, last - buffer, " Priority: %s",
// 					core::Download::priority_to_string(m_download->variables()->get("priority").as_value())),
// 			       0);
  m_canvas->print(0, 1, "%s", buffer);

  position = print_download_status(buffer, last, m_download);
  m_canvas->print(0, 2, "[%c:%i] %s",
		  m_download->get_download().is_tracker_busy() ? 'C' : ' ',
		  (int)(m_download->get_download().tracker_timeout() / 1000000),
		  buffer);
}

}
