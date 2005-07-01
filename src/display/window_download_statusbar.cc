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

#include "canvas.h"
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
  utils::displayScheduler.insert(&m_taskUpdate, utils::Timer::cache().round_seconds() + 1000000);

  m_canvas->erase();

  if (m_download->get_download().get_chunks_done() != m_download->get_download().get_chunks_total() || !m_download->get_download().is_open())
    m_canvas->print(0, 0, "Torrent: %.1f / %.1f MiB Rate: %5.1f / %5.1f KiB Uploaded: %.1f MiB",
		    (double)m_download->get_download().get_bytes_done() / (double)(1 << 20),
		    (double)m_download->get_download().get_bytes_total() / (double)(1 << 20),
		    (double)m_download->get_download().get_rate_up() / 1024.0,
		    (double)m_download->get_download().get_rate_down() / 1024.0,
		    (double)m_download->get_download().get_bytes_up() / (double)(1 << 20));
 
  else
    m_canvas->print(0, 0, "Torrent: Done %.1f MiB Rate: %5.1f / %5.1f KiB Uploaded: %.1f MiB",
		    (double)m_download->get_download().get_bytes_total() / (double)(1 << 20),
		    (double)m_download->get_download().get_rate_up() / 1024.0,
		    (double)m_download->get_download().get_rate_down() / 1024.0,
		    (double)m_download->get_download().get_bytes_up() / (double)(1 << 20));
    
  m_canvas->print(0, 1, "Peers: %i(%i) Min/Max: %i/%i Uploads: %i",
		  (int)m_download->get_download().get_peers_connected(),
		  (int)m_download->get_download().get_peers_not_connected(),
		  (int)m_download->get_download().get_peers_min(),
		  (int)m_download->get_download().get_peers_max(),
		  (int)m_download->get_download().get_uploads_max());

  m_canvas->print(0, 2, "[%c:%i] %s",
		  m_download->get_download().is_tracker_busy() ? 'C' : ' ',
		  (int)(m_download->get_download().get_tracker_timeout() / 1000000),
		  print_download_status(m_download).c_str());
}

}
