// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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
#include <torrent/data/transfer_list.h>
#include <torrent/tracker_controller.h>
#include <torrent/tracker_list.h>
#include <torrent/peer/connection_list.h>
#include <torrent/peer/peer_list.h>

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "window_download_statusbar.h"

#include "core/download.h"

namespace display {

WindowDownloadStatusbar::WindowDownloadStatusbar(core::Download* d) :
  Window(new Canvas, 0, 0, 3, extent_full, extent_static),
  m_download(d) {
}

void
WindowDownloadStatusbar::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());

  m_canvas->erase();

  char buffer[m_canvas->width()];
  char* last = buffer + m_canvas->width() - 2;

  print_download_info(buffer, last, m_download);
  m_canvas->print(0, 0, "%s", buffer);

  snprintf(buffer, last - buffer, "Peers: %i(%i) Min/Max: %i/%i Slots: U:%i/%i D:%i/%i U/I/C/A: %i/%i/%i/%i Unchoked: %u/%u Failed: %i",
           (int)m_download->download()->connection_list()->size(),
           (int)m_download->download()->peer_list()->available_list_size(),
           (int)m_download->download()->connection_list()->min_size(),
           (int)m_download->download()->connection_list()->max_size(),
           (int)m_download->download()->uploads_min(),
           (int)m_download->download()->uploads_max(),
           (int)m_download->download()->downloads_min(),
           (int)m_download->download()->downloads_max(),
           (int)m_download->download()->peers_currently_unchoked(),
           (int)m_download->download()->peers_currently_interested(),
           (int)m_download->download()->peers_complete(),
           (int)m_download->download()->peers_accounted(),
           (int)m_download->info()->upload_unchoked(),
           (int)m_download->info()->download_unchoked(),
           (int)m_download->download()->transfer_list()->failed_count());

  m_canvas->print(0, 1, "%s", buffer);

  print_download_status(buffer, last, m_download);
  m_canvas->print(0, 2, "[%c:%i] %s",
                  m_download->tracker_list()->has_active() ? 'C' : ' ',
                  (int)(m_download->download()->tracker_controller()->seconds_to_next_timeout()),
                  buffer);
}

}
