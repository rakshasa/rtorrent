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
#include <rak/socket_address.h>
#include <rak/string_manip.h>
#include <torrent/rate.h>
#include <torrent/connection_manager.h>

#include "core/download.h"

#include "globals.h"

#include "canvas.h"
#include "client_info.h"
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
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());
  m_canvas->erase();

  int y = 0;
  torrent::Download d = m_download->get_download();

  m_canvas->print(0, y++, "Hash:    %s", rak::transform_hex(d.info_hash()).c_str());
  m_canvas->print(0, y++, "Id:      %s", rak::copy_escape_html(d.local_id()).c_str());
  m_canvas->print(0, y++, "Chunks:  %u / %u * %u",
		  d.chunks_done(),
		  d.chunks_total(),
		  d.chunks_size());

  char buffer[32], *position;
  position = print_ddmmyyyy(buffer, buffer + 32, static_cast<time_t>(d.creation_date()));
  position = print_string(position, buffer + 32, " ");
  position = print_hhmmss(position, buffer + 32, static_cast<time_t>(d.creation_date()));

  m_canvas->print(0, y++, "Created: %s", buffer);

  y++;

  m_canvas->print(0, y++, "Connection Type: %s ( %s / %s )",
		  m_download->variables()->get("connection_current").as_string().c_str(),
		  m_download->variables()->get("connection_seed").as_string().c_str(),
		  m_download->variables()->get("connection_leech").as_string().c_str());
  m_canvas->print(0, y++, "Priority:        %u", torrent::download_priority(m_download->get_download()));

  m_canvas->print(0, y++, "Directory:       %s", m_download->variable_string("directory").c_str());
  m_canvas->print(0, y++, "Tied to file:    %s", m_download->variable_string("tied_to_file").c_str());

  y++;

  // Temporary.
//   m_canvas->print(0, y++, "SndBuf:          %u", torrent::connection_manager()->send_buffer_size());
//   m_canvas->print(0, y++, "RcvBuf:          %u", torrent::connection_manager()->receive_buffer_size());

//   y++;

  if (*m_focus == m_list->end()) {
    m_canvas->print(0, y++, "No peer in focus");

    return;
  }

  m_canvas->print(0, y++, "*** Peer Info ***");

  m_canvas->print(0, y++, "IP: %s:%hu",
		  rak::socket_address::cast_from((*m_focus)->address())->address_str().c_str(),
		  rak::socket_address::cast_from((*m_focus)->address())->port());

  m_canvas->print(0, y++, "ID: %s" , rak::copy_escape_html((*m_focus)->id()).c_str());

  char buf[128];
  control->client_info()->print(buf, buf + 128, (*m_focus)->id().c_str());

  m_canvas->print(0, y++, "Client: %s" , buf);

  m_canvas->print(0, y++, "Options: %s" , rak::transform_hex(std::string((*m_focus)->options(), 8)).c_str());
  m_canvas->print(0, y++, "Snubbed: %s", (*m_focus)->is_snubbed() ? "yes" : "no");
  m_canvas->print(0, y++, "Connected: %s", (*m_focus)->is_incoming() ? "remote" : "local");

  m_canvas->print(0, y++, "Done: %i%", done_percentage(**m_focus));

  m_canvas->print(0, y++, "Rate: %5.1f/%5.1f KB Total: %.1f/%.1f MB",
		  (double)(*m_focus)->up_rate()->rate() / (double)(1 << 10),
		  (double)(*m_focus)->down_rate()->rate() / (double)(1 << 10),
		  (double)(*m_focus)->up_rate()->total() / (double)(1 << 20),
		  (double)(*m_focus)->down_rate()->total() / (double)(1 << 20));
}

int
WindowPeerInfo::done_percentage(torrent::Peer& p) {
  int chunks = m_download->get_download().chunks_total();

  return chunks ? (100 * p.chunks_done()) / chunks : 0;
}

}
