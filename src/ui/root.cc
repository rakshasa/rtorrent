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

#include <sigc++/bind.h>

#include "control.h"
#include "download_list.h"

#include "root.h"

namespace ui {

void
Root::init() {
  setup_keys();

  m_downloadList = new DownloadList(m_control);

  m_downloadList->activate();
  m_downloadList->slot_open_uri(sigc::mem_fun(m_control->get_core(), &core::Manager::insert));
}

void
Root::cleanup() {
  delete m_downloadList;

  m_control->get_input().erase(&m_bindings);
}

void
Root::setup_keys() {
  m_control->get_input().push_back(&m_bindings);

  m_bindings[KEY_RESIZE] = sigc::mem_fun(m_control->get_display(), &display::Manager::adjust_layout);
  m_bindings['\x11']     = sigc::bind(sigc::mem_fun(*this, &Root::set_shutdown_received), true);
}

}
