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
#include <rak/functional.h>

#include "core/download.h"
#include "input/bindings.h"
#include "display/window_title.h"
#include "display/window_download_statusbar.h"
#include "display/window_statusbar.h"

#include "control.h"
#include "download.h"
#include "element_file_list.h"
#include "element_peer_info.h"
#include "element_peer_list.h"
#include "element_tracker_list.h"

namespace ui {

Download::Download(DPtr d, Control* c) :
  m_download(d),
  m_state(DISPLAY_MAX_SIZE),

  m_windowTitle(new WTitle(d->get_download().get_name())),
  m_windowDownloadStatus(new WDownloadStatus(d)),
  m_windowMainStatus(new WMainStatus(&c->get_core())),

  m_window(c->get_display().end()),

  m_control(c),
  m_bindings(new input::Bindings) {

  m_focus = m_peers.end();

  m_uiArray[DISPLAY_PEER_LIST]    = new ElementPeerList(d, &m_peers, &m_focus);
  m_uiArray[DISPLAY_PEER_INFO]    = new ElementPeerInfo(d, &m_peers, &m_focus);
  m_uiArray[DISPLAY_FILE_LIST]    = new ElementFileList(d);
  m_uiArray[DISPLAY_TRACKER_LIST] = new ElementTrackerList(d);

  bind_keys();

  m_download->get_download().peer_list(m_peers);

  m_connPeerConnected    = m_download->get_download().signal_peer_connected(sigc::mem_fun(*this, &Download::receive_peer_connected));
  m_connPeerDisconnected = m_download->get_download().signal_peer_disconnected(sigc::mem_fun(*this, &Download::receive_peer_disconnected));
}

Download::~Download() {
  if (m_window != m_control->get_display().end())
    throw std::logic_error("ui::Download::~Download() called on an active object");

  std::for_each(m_uiArray, m_uiArray + DISPLAY_MAX_SIZE, rak::call_delete<ElementBase>());

  delete m_windowTitle;
  delete m_windowDownloadStatus;
  delete m_windowMainStatus;

  delete m_bindings;

  m_connPeerConnected.disconnect();
  m_connPeerDisconnected.disconnect();
}

void
Download::activate() {
  if (m_window != m_control->get_display().end())
    throw std::logic_error("ui::Download::activate() called on an already activated object");

  m_window = m_control->get_display().insert(m_control->get_display().begin(), NULL);

  m_control->get_display().insert(m_control->get_display().begin(), m_windowTitle);
  m_control->get_display().insert(m_control->get_display().end(), m_windowDownloadStatus);
  m_control->get_display().insert(m_control->get_display().end(), m_windowMainStatus);

  m_control->get_input().push_front(m_bindings);

  activate_display(DISPLAY_PEER_LIST);
}

void
Download::disable() {
  if (m_window == m_control->get_display().end())
    throw std::logic_error("ui::Download::disable() called on an already disabled object");

  disable_display();

  m_control->get_display().erase(m_window);
  m_control->get_display().erase(m_windowTitle);
  m_control->get_display().erase(m_windowDownloadStatus);
  m_control->get_display().erase(m_windowMainStatus);

  m_window = m_control->get_display().end();

  m_control->get_input().erase(m_bindings);
}

void
Download::activate_display(Display d) {
  if (m_window == m_control->get_display().end())
    throw std::logic_error("ui::Download::activate_display(...) could not find previous display iterator");

  if (d >= DISPLAY_MAX_SIZE)
    throw std::logic_error("ui::Download::activate_display(...) out of bounds");

  m_state = d;
  m_uiArray[d]->activate(m_control, m_window);

  m_control->get_display().adjust_layout();
}

// Does not delete disabled window.
void
Download::disable_display() {
  m_uiArray[m_state]->disable(m_control);

  m_state   = DISPLAY_MAX_SIZE;
  *m_window = NULL;
}

void
Download::receive_next() {
  if (m_focus != m_peers.end())
    ++m_focus;
  else
    m_focus = m_peers.begin();

  mark_dirty();
}

void
Download::receive_prev() {
  if (m_focus != m_peers.begin())
    --m_focus;
  else
    m_focus = m_peers.end();

  mark_dirty();
}

void
Download::receive_peer_connected(torrent::Peer p) {
  m_peers.push_back(p);
}

void
Download::receive_peer_disconnected(torrent::Peer p) {
  PList::iterator itr = std::find(m_peers.begin(), m_peers.end(), p);

  if (itr == m_peers.end())
    throw std::logic_error("Download::receive_peer_disconnected(...) received a peer we don't have in our list");

  if (itr == m_focus)
    m_focus = m_peers.erase(itr);
  else
    m_peers.erase(itr);
}

void
Download::receive_throttle(int t) {
  m_windowMainStatus->mark_dirty();

  torrent::set(torrent::THROTTLE_ROOT_CONST_RATE, torrent::get(torrent::THROTTLE_ROOT_CONST_RATE) + t * 1024);
}

void
Download::receive_max_uploads(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->get_download().set_uploads_max(std::max(m_download->get_download().get_uploads_max() + t, (uint32_t)2));
}

void
Download::receive_min_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->get_download().set_peers_min(std::max(m_download->get_download().get_peers_min() + t, (uint32_t)5));
}

void
Download::receive_max_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->get_download().set_peers_max(std::max(m_download->get_download().get_peers_max() + t, (uint32_t)5));
}

void
Download::receive_change(Display d) {
  if (d == m_state)
    return;

  disable_display();
  activate_display(d);
}

void
Download::receive_snub_peer() {
  if (m_focus == m_peers.end())
    return;

  m_focus->set_snubbed(!m_focus->get_snubbed());

  mark_dirty();
}

void
Download::bind_keys() {
  (*m_bindings)['a'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_throttle), 1);
  (*m_bindings)['z'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_throttle), -1);
  (*m_bindings)['s'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_throttle), 5);
  (*m_bindings)['x'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_throttle), -5);
  (*m_bindings)['d'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_throttle), 50);
  (*m_bindings)['c'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_throttle), -50);

  (*m_bindings)['1'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_max_uploads), -1);
  (*m_bindings)['2'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_max_uploads), 1);
  (*m_bindings)['3'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_min_peers), -5);
  (*m_bindings)['4'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_min_peers), 5);
  (*m_bindings)['5'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_max_peers), -5);
  (*m_bindings)['6'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_max_peers), 5);

  (*m_bindings)['t'] = sigc::bind(sigc::mem_fun(m_download->get_download(), &torrent::Download::set_tracker_timeout), 2 * 1000000);

  (*m_bindings)['p'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_change), DISPLAY_PEER_INFO);
  (*m_bindings)['o'] = sigc::bind(sigc::mem_fun(*this, &Download::receive_change), DISPLAY_TRACKER_LIST);

  (*m_bindings)[KEY_UP]   = sigc::mem_fun(*this, &Download::receive_prev);
  (*m_bindings)[KEY_DOWN] = sigc::mem_fun(*this, &Download::receive_next);

  // Key bindings for sub-ui's.
  m_uiArray[DISPLAY_PEER_LIST]->get_bindings()[KEY_RIGHT] = sigc::bind(sigc::mem_fun(*this, &Download::receive_change), DISPLAY_FILE_LIST);
  m_uiArray[DISPLAY_PEER_INFO]->get_bindings()[' ']       = sigc::bind(sigc::mem_fun(*this, &Download::receive_change), DISPLAY_PEER_LIST);
  m_uiArray[DISPLAY_FILE_LIST]->get_bindings()[KEY_LEFT]  = sigc::bind(sigc::mem_fun(*this, &Download::receive_change), DISPLAY_PEER_LIST);
  m_uiArray[DISPLAY_TRACKER_LIST]->get_bindings()[' ']    = sigc::bind(sigc::mem_fun(*this, &Download::receive_change), DISPLAY_PEER_LIST);

  m_uiArray[DISPLAY_PEER_LIST]->get_bindings()['*'] = sigc::mem_fun(*this, &Download::receive_snub_peer);
  m_uiArray[DISPLAY_PEER_INFO]->get_bindings()['*'] = sigc::mem_fun(*this, &Download::receive_snub_peer);
}

void
Download::mark_dirty() {
  (*m_window)->mark_dirty();
}

}
