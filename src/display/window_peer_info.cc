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

#include "utils/parse.h"
#include "canvas.h"
#include "utils.h"
#include "window_peer_info.h"

namespace display {

WindowPeerInfo::WindowPeerInfo(core::Download* d, PList* l, PList::iterator* f) :
  Window(new Canvas, true),
  m_download(d),
  m_list(l),
  m_focus(f) {
}

void
WindowPeerInfo::redraw() {
  utils::displayScheduler.insert(&m_taskUpdate, utils::Timer::cache().round_seconds() + 1000000);
  m_canvas->erase();

  int y = 0;
  torrent::Download d = m_download->get_download();

  m_canvas->print(0, y++, "Hash:    %s", utils::string_to_hex(d.get_hash()).c_str());
  m_canvas->print(0, y++, "Id:      %s", utils::escape_string(d.get_id()).c_str());
  m_canvas->print(0, y++, "Chunks:  %u / %u * %u",
		  d.get_chunks_done(),
		  d.get_chunks_total(),
		  d.get_chunks_size());
  m_canvas->print(0, y++, "Created: %s",
		  print_ddmmyyyy(static_cast<time_t>(d.get_creation_date())).c_str());

  y++;

  if (*m_focus == m_list->end()) {
    m_canvas->print(0, y++, "No peer in focus");

    return;
  }

  m_canvas->print(0, y++, "*** Peer Info ***");

  m_canvas->print(0, y++, "DNS: %s:%hu", (*m_focus)->get_dns().c_str(), (*m_focus)->get_port());
  m_canvas->print(0, y++, "Id: %s" , utils::escape_string((*m_focus)->get_id()).c_str());
  m_canvas->print(0, y++, "Snubbed: %s", (*m_focus)->get_snubbed() ? "Yes" : "No");

  m_canvas->print(0, y++, "Done: %i%", done_percentage(**m_focus));

  m_canvas->print(0, y++, "Rate: %5.1f/%5.1f KB Total: %.1f/%.1f MB",
		  (double)(*m_focus)->get_rate_up() / (double)(1 << 10),
		  (double)(*m_focus)->get_rate_down() / (double)(1 << 10),
		  (double)(*m_focus)->get_transfered_up() / (double)(1 << 20),
		  (double)(*m_focus)->get_transfered_down() / (double)(1 << 20));
}

int
WindowPeerInfo::done_percentage(torrent::Peer& p) {
  int chunks = m_download->get_download().get_chunks_total();

  return chunks ? (100 * p.get_chunks_done()) / chunks : 0;
}

}
