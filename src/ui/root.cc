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
#include <sigc++/bind.h>
#include <torrent/torrent.h>

#include "control.h"
#include "download_list.h"
#include "display/window_statusbar.h"

#include "root.h"

namespace ui {

Root::Root(Control* c) :
  m_shutdownReceived(false),
  m_control(c),
  m_downloadList(NULL),
  m_windowStatus(NULL) {
}

void
Root::init() {
  if (m_downloadList != NULL)
    throw std::logic_error("Root::init() called twice on the same object");

  setup_keys();

  m_windowStatus = new WStatus(&m_control->get_core());
  m_downloadList = new DownloadList(m_control);

  m_control->get_display().push_back(m_windowStatus);

  m_downloadList->activate();
  m_downloadList->slot_open_uri(sigc::mem_fun(m_control->get_core(), &core::Manager::insert));
}

void
Root::cleanup() {
  if (m_downloadList->is_active())
    m_downloadList->disable();

  m_control->get_display().erase(m_windowStatus);

  delete m_downloadList;
  delete m_windowStatus;

  m_control->get_input().erase(&m_bindings);
}

void
Root::setup_keys() {
  m_control->get_input().push_back(&m_bindings);

  m_bindings['a']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_write_throttle), 1);
  m_bindings['z']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_write_throttle), -1);
  m_bindings['s']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_write_throttle), 5);
  m_bindings['x']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_write_throttle), -5);
  m_bindings['d']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_write_throttle), 50);
  m_bindings['c']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_write_throttle), -50);

  m_bindings['A']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_read_throttle), 1);
  m_bindings['Z']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_read_throttle), -1);
  m_bindings['S']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_read_throttle), 5);
  m_bindings['X']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_read_throttle), -5);
  m_bindings['D']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_read_throttle), 50);
  m_bindings['C']           = sigc::bind(sigc::mem_fun(*this, &Root::receive_read_throttle), -50);

  m_bindings[KEY_RESIZE]    = sigc::mem_fun(m_control->get_display(), &display::Manager::adjust_layout);
  m_bindings['\x11']        = sigc::bind(sigc::mem_fun(*this, &Root::set_shutdown_received), true);
}

void
Root::receive_read_throttle(int t) {
  //m_downloadList->mark_dirty();

  torrent::set_read_throttle(torrent::get_read_throttle() + t * 1024);
}

void
Root::receive_write_throttle(int t) {
  //m_downloadList->mark_dirty();

  torrent::set_write_throttle(torrent::get_write_throttle() + t * 1024);
}

}
