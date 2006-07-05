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
#include <string.h>
#include <sigc++/bind.h>
#include <torrent/torrent.h>

#include "display/window_statusbar.h"
#include "input/manager.h"
#include "utils/variable_map.h"

#include "control.h"
#include "download_list.h"

#include "root.h"

namespace ui {

Root::Root() :
  m_control(NULL),
  m_downloadList(NULL),
  m_windowStatusbar(NULL) {
}

void
Root::init(Control* c) {
  if (m_control != NULL)
    throw std::logic_error("Root::init() called twice on the same object");

  m_control = c;
  setup_keys();

  m_windowStatusbar = new WStatusbar(m_control);
  m_downloadList =    new DownloadList(m_control);

  m_control->display()->push_back(m_windowStatusbar);

  m_downloadList->activate();
  //  m_downloadList->slot_open_uri(sigc::mem_fun(m_control->get_core(), &core::Manager::insert));
}

void
Root::cleanup() {
  if (m_control == NULL)
    throw std::logic_error("Root::cleanup() called twice on the same object");

  if (m_downloadList->is_active())
    m_downloadList->disable();

  m_control->display()->erase(m_windowStatusbar);

  delete m_downloadList;
  delete m_windowStatusbar;

  m_control->input()->erase(&m_bindings);
  m_control = NULL;
}

void
Root::setup_keys() {
  m_control->input()->push_back(&m_bindings);

  if (strcasecmp(control->variable()->get_string("key_layout").c_str(), "azerty") == 0) {
    m_bindings['q']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 1);
    m_bindings['w']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -1);
    m_bindings['Q']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 1);
    m_bindings['W']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -1);

  } else {
    m_bindings['a']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 1);
    m_bindings['z']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -1);
    m_bindings['A']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 1);
    m_bindings['Z']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -1);
  }

  m_bindings['s']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 5);
  m_bindings['x']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -5);
  m_bindings['S']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 5);
  m_bindings['X']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -5);

  m_bindings['d']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 50);
  m_bindings['c']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -50);
  m_bindings['D']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 50);
  m_bindings['C']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -50);

  m_bindings['\x0C']        = sigc::mem_fun(m_control->display(), &display::Manager::force_redraw); // ^L
  m_bindings['\x11']        = sigc::mem_fun(m_control, &Control::receive_normal_shutdown); // ^Q
}

void
Root::set_down_throttle(unsigned int throttle) {
  if (m_windowStatusbar != NULL)
    m_windowStatusbar->mark_dirty();

  torrent::set_down_throttle(throttle * 1024);
}

void
Root::set_up_throttle(unsigned int throttle) {
  if (m_windowStatusbar != NULL)
    m_windowStatusbar->mark_dirty();

  torrent::set_up_throttle(throttle * 1024);

  if (throttle == 0)
    torrent::set_max_unchoked(0);

  else if (throttle <= 10)
    torrent::set_max_unchoked(1 + throttle / 1);

  else
    torrent::set_max_unchoked(10 + throttle / 5);
}

void
Root::adjust_down_throttle(int throttle) {
  set_down_throttle(std::max<int>(torrent::down_throttle() / 1024 + throttle, 0));
}

void
Root::adjust_up_throttle(int throttle) {
  set_up_throttle(std::max<int>(torrent::up_throttle() / 1024 + throttle, 0));
}

}
