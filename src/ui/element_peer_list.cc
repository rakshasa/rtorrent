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

#include <sigc++/bind.h>
#include <torrent/exceptions.h>

#include "display/frame.h"
#include "display/manager.h"
#include "display/window_peer_info.h"
#include "display/window_peer_list.h"
#include "input/manager.h"

#include "control.h"
#include "element_peer_list.h"

namespace ui {

ElementPeerList::ElementPeerList(core::Download* d) :
  m_download(d),
  m_state(DISPLAY_MAX_SIZE) {

  m_listItr = m_list.end();

  m_download->download()->peer_list(m_list);

  m_connPeerConnected    = m_download->download()->signal_peer_connected(sigc::mem_fun(*this, &ElementPeerList::receive_peer_connected));
  m_connPeerDisconnected = m_download->download()->signal_peer_disconnected(sigc::mem_fun(*this, &ElementPeerList::receive_peer_disconnected));

  m_bindings['k']       = sigc::mem_fun(this, &ElementPeerList::receive_disconnect_peer);
  m_bindings['*']       = sigc::mem_fun(this, &ElementPeerList::receive_snub_peer);
  m_bindings[KEY_UP]    = sigc::mem_fun(this, &ElementPeerList::receive_prev);
  m_bindings[KEY_DOWN]  = sigc::mem_fun(this, &ElementPeerList::receive_next);
  m_bindings[KEY_RIGHT] = sigc::bind(sigc::mem_fun(this, &ElementPeerList::activate_display), DISPLAY_INFO);
}

ElementPeerList::~ElementPeerList() {
  m_connPeerConnected.disconnect();
  m_connPeerDisconnected.disconnect();
}

void
ElementPeerList::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::client_error("ui::ElementPeerList::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_frame = frame;
  m_focus = focus;

  m_window[DISPLAY_LIST] = new display::WindowPeerList(m_download, &m_list, &m_listItr);
  m_window[DISPLAY_INFO] = new display::WindowPeerInfo(m_download, &m_list, &m_listItr);

  activate_display(DISPLAY_LIST);
}

void
ElementPeerList::disable() {
  if (!is_active())
    throw torrent::client_error("ui::ElementPeerList::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  activate_display(DISPLAY_MAX_SIZE);

  m_frame->clear();
  m_frame = NULL;

  std::for_each(m_window, m_window + DISPLAY_MAX_SIZE, rak::call_delete<display::Window>());
}

void
ElementPeerList::activate_display(Display display) {
  if (display == m_state)
    return;

  switch (m_state) {
  case DISPLAY_INFO:
  case DISPLAY_LIST:
    m_bindings.erase(KEY_LEFT);

    m_window[m_state]->set_active(false);
    m_frame->clear();
    break;

  case DISPLAY_MAX_SIZE:
    break;
  }

  m_state = display;

  switch (m_state) {
  case DISPLAY_INFO:
    m_bindings[KEY_LEFT] = sigc::bind(sigc::mem_fun(this, &ElementPeerList::activate_display), DISPLAY_LIST);

    m_window[m_state]->set_active(true);
    m_frame->initialize_window(m_window[m_state]);
    break;

  case DISPLAY_LIST:
    m_bindings[KEY_LEFT] = sigc::mem_fun(&m_slotExit, &slot_type::operator());

    m_window[m_state]->set_active(true);
    m_frame->initialize_window(m_window[m_state]);
    break;

  case DISPLAY_MAX_SIZE:
    break;
  }

  control->display()->adjust_layout();
}

void
ElementPeerList::receive_next() {
  if (m_listItr != m_list.end())
    ++m_listItr;
  else
    m_listItr = m_list.begin();

  m_window[m_state]->mark_dirty();
}

void
ElementPeerList::receive_prev() {
  if (m_listItr != m_list.begin())
    --m_listItr;
  else
    m_listItr = m_list.end();

  m_window[m_state]->mark_dirty();
}

void
ElementPeerList::receive_disconnect_peer() {
  if (m_listItr == m_list.end())
    return;

  m_download->download()->disconnect_peer(*m_listItr);

  m_window[m_state]->mark_dirty();
}

void
ElementPeerList::receive_peer_connected(torrent::Peer p) {
  m_list.push_back(p);
}

void
ElementPeerList::receive_peer_disconnected(torrent::Peer p) {
  PList::iterator itr = std::find(m_list.begin(), m_list.end(), p);

  if (itr == m_list.end())
    throw torrent::client_error("ElementPeerList::receive_peer_disconnected(...) itr == m_list.end().");

  if (itr == m_listItr)
    m_listItr = m_list.erase(itr);
  else
    m_list.erase(itr);
}

void
ElementPeerList::receive_snub_peer() {
  if (m_listItr == m_list.end())
    return;

  m_listItr->set_snubbed(!m_listItr->is_snubbed());

  m_window[m_state]->mark_dirty();
}

}
