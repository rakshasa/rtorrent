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
#include <torrent/rate.h>

#include "core/download.h"
#include "rak/algorithm.h"

#include "canvas.h"
#include "client_info.h"
#include "window_peer_list.h"

namespace display {

WindowPeerList::WindowPeerList(core::Download* d, PList* l, PList::iterator* f) :
  Window(new Canvas, true),
  m_download(d),
  m_list(l),
  m_focus(f) {
}

void
WindowPeerList::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());
  m_canvas->erase();

  int x = 2;
  int y = 0;

  m_canvas->print(x, y, "IP");     x += 16;
  m_canvas->print(x, y, "UP");      x += 7;
  m_canvas->print(x, y, "DOWN");    x += 7;
  m_canvas->print(x, y, "PEER");    x += 7;
  m_canvas->print(x, y, "C/RE/LO"); x += 9;
  m_canvas->print(x, y, "QS");      x += 6;
  m_canvas->print(x, y, "DONE");    x += 6;
  m_canvas->print(x, y, "REQ");     x += 6;
  m_canvas->print(x, y, "SNUB");

  ++y;

  if (m_list->empty())
    return;

  typedef std::pair<PList::iterator, PList::iterator> Range;

  Range range = rak::advance_bidirectional(m_list->begin(),
					   *m_focus != m_list->end() ? *m_focus : m_list->begin(),
					   m_list->end(),
					   m_canvas->get_height() - y);

  if (m_download->get_download().chunks_total() <= 0)
    throw std::logic_error("WindowPeerList::redraw() m_slotChunksTotal() returned invalid value");

  while (range.first != range.second) {
    torrent::Peer& p = *range.first;

    x = 0;

    m_canvas->print(x, y, "%c %s",
		    range.first == *m_focus ? '*' : ' ',
		    rak::socket_address::cast_from(p.address())->address_str().c_str());
    x += 18;

    m_canvas->print(x, y, "%.1f", (double)p.up_rate()->rate() / 1024); x += 7;
    m_canvas->print(x, y, "%.1f", (double)p.down_rate()->rate() / 1024); x += 7;
    m_canvas->print(x, y, "%.1f", (double)p.peer_rate()->rate() / 1024); x += 7;

    m_canvas->print(x, y, "%c/%c%c/%c%c",
		    p.is_incoming() ? 'r' : 'l',
		    p.is_remote_choked() ? 'c' : 'u',
		    p.is_remote_interested() ? 'i' : 'n',
		    p.is_local_choked() ? 'c' : 'u',
		    p.is_local_interested() ? 'i' : 'n');
    x += 9;

    m_canvas->print(x, y, "%i/%i",
		    p.outgoing_queue_size(),
		    p.incoming_queue_size());
    x += 6;

    m_canvas->print(x, y, "%3i", done_percentage(*range.first));
    x += 6;

    if (p.incoming_queue_size())
      m_canvas->print(x, y, "%i", p.incoming_index(0));

    x += 6;

    if (p.is_snubbed())
      m_canvas->print(x, y, "*");

    x += 5;

    char buf[128];
    control->client_info()->print(buf, buf + 128, p.id().c_str());

    m_canvas->print(x, y, "%s", buf);

    ++y;
    ++range.first;
  }
}

int
WindowPeerList::done_percentage(torrent::Peer& p) {
  int chunks = m_download->get_download().chunks_total();

  return chunks ? (100 * p.chunks_done()) / chunks : 0;
}

}
