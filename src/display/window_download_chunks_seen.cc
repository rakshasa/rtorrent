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
#include <rak/string_manip.h>

#include "core/download.h"

#include "window_download_chunks_seen.h"

namespace display {

WindowDownloadChunksSeen::WindowDownloadChunksSeen(core::Download* d) :
  Window(new Canvas, true),
  m_download(d) {
}

void
WindowDownloadChunksSeen::redraw() {
  // TODO: Make this depend on tracker signal.
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(10)).round_seconds());
  m_canvas->erase();

  if (m_canvas->get_height() < 3 || m_canvas->get_width() < 10)
    return;

  m_canvas->print(2, 0, "Chunks seen: [C/A %i/%i]",
		  (int)m_download->get_download().peers_complete(),
		  (int)m_download->get_download().peers_accounted());

  const uint8_t* seen = m_download->get_download().chunks_seen();
  const uint8_t* last = seen + m_download->get_download().chunks_total();

  if (seen == NULL) {
    m_canvas->print(2, 2, "Not available.");
    return;
  }

  for (int y = 1; y < m_canvas->get_height() && seen != last; ++y) {

    for (int x = 2; x < m_canvas->get_width() && seen != last; ++x) {
      m_canvas->print(x, y, "%c", rak::value_to_hexchar<0>(std::min<uint8_t>(*seen, 0xF)));

      seen++;
    }
  }
}

}
