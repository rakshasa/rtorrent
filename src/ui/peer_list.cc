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

#include "display/window_peer_list.h"

#include "control.h"
#include "peer_list.h"

namespace ui {

PeerList::PeerList(core::Download* d, PList* l, PList::iterator* f) :
  m_download(d),
  m_window(NULL),
  m_list(l),
  m_focus(f) {

}

void
PeerList::activate(Control* c, MItr mItr) {
  if (m_window != NULL)
    throw std::logic_error("ui::PeerList::activate(...) called on an object in the wrong state");

  c->get_input().push_front(&m_bindings);

  *mItr = m_window = new WPeerList(m_download, m_list, m_focus);
}

void
PeerList::disable(Control* c) {
  if (m_window == NULL)
    throw std::logic_error("ui::PeerList::disable(...) called on an object in the wrong state");

  c->get_input().erase(&m_bindings);

  delete m_window;
  m_window = NULL;
}

}
