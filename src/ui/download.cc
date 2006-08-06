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
#include <rak/functional.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/tracker_list.h>

#include "core/download.h"
#include "input/manager.h"
#include "display/window_title.h"
#include "display/window_download_statusbar.h"

#include "control.h"
#include "download.h"
#include "root.h"
#include "element_file_list.h"
#include "element_menu.h"
#include "element_peer_info.h"
#include "element_peer_list.h"
#include "element_tracker_list.h"
#include "element_chunks_seen.h"
#include "element_transfer_list.h"

namespace ui {

Download::Download(DPtr d) :
  m_download(d),
  m_state(DISPLAY_MAX_SIZE),
  m_focusDisplay(false) {

  m_focus = m_peers.end();

  m_windowDownloadStatus = new WDownloadStatus(d);
  m_windowDownloadStatus->set_bottom(true);

  ElementMenu* elementMenu = new ElementMenu;

  elementMenu->push_back("Peer List",     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_PEER_LIST));
//   elementMenu->push_back("Peer Info",     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_PEER_INFO));
  elementMenu->push_back("File List",     sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_FILE_LIST));
  elementMenu->push_back("Tracker List",  sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_TRACKER_LIST));
  elementMenu->push_back("Chunks Seen",   sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_CHUNKS_SEEN));
  elementMenu->push_back("Transfer List", sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_TRANSFER_LIST));

  elementMenu->focus_next();

  m_uiArray[DISPLAY_MENU]          = elementMenu;
  m_uiArray[DISPLAY_PEER_LIST]     = new ElementPeerList(d, &m_peers, &m_focus);
  m_uiArray[DISPLAY_PEER_INFO]     = new ElementPeerInfo(d, &m_peers, &m_focus);
  m_uiArray[DISPLAY_FILE_LIST]     = new ElementFileList(d);
  m_uiArray[DISPLAY_TRACKER_LIST]  = new ElementTrackerList(d);
  m_uiArray[DISPLAY_CHUNKS_SEEN]   = new ElementChunksSeen(d);
  m_uiArray[DISPLAY_TRANSFER_LIST] = new ElementTransferList(d);

  bind_keys();

  m_download->download()->peer_list(m_peers);

  m_connPeerConnected    = m_download->download()->signal_peer_connected(sigc::mem_fun(*this, &Download::receive_peer_connected));
  m_connPeerDisconnected = m_download->download()->signal_peer_disconnected(sigc::mem_fun(*this, &Download::receive_peer_disconnected));
}

Download::~Download() {
  if (is_active())
    throw torrent::client_error("ui::Download::~Download() called on an active object.");

  m_connPeerConnected.disconnect();
  m_connPeerDisconnected.disconnect();

  std::for_each(m_uiArray, m_uiArray + DISPLAY_MAX_SIZE, rak::call_delete<ElementBase>());

  delete m_windowDownloadStatus;
}

void
Download::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::client_error("ui::Download::activate() called on an already activated object.");

  control->input()->push_back(&m_bindings);

  m_frame = frame;
  m_frame->initialize_row(2);

  m_frame->frame(1)->initialize_window(m_windowDownloadStatus);
  m_windowDownloadStatus->set_active(true);

  activate_display_menu(DISPLAY_PEER_LIST);
}

void
Download::disable() {
  if (!is_active())
    throw torrent::client_error("ui::Download::disable() called on an already disabled object.");

  control->input()->erase(&m_bindings);

  activate_display_focus(DISPLAY_MAX_SIZE);

  m_windowDownloadStatus->set_active(false);
  m_frame->clear();

  m_frame = NULL;
}

void
Download::activate_display(Display displayType, bool focusDisplay) {
  if (!is_active())
    throw torrent::client_error("ui::Download::activate_display(...) !is_active().");

  if (displayType > DISPLAY_MAX_SIZE)
    throw torrent::client_error("ui::Download::activate_display(...) out of bounds");

  if (focusDisplay == m_focusDisplay && displayType == m_state)
    return;

  display::Frame* frame = m_frame->frame(0);

  // Cleanup previous state.
  switch (m_state) {
  case DISPLAY_MENU:
    break;

  case DISPLAY_PEER_LIST:
  case DISPLAY_PEER_INFO:
  case DISPLAY_FILE_LIST:
  case DISPLAY_TRACKER_LIST:
  case DISPLAY_CHUNKS_SEEN:
  case DISPLAY_TRANSFER_LIST:
    m_uiArray[DISPLAY_MENU]->disable();
    m_uiArray[m_state]->disable();

    frame->clear();
    break;

  case DISPLAY_MAX_SIZE:
    break;
  }

  m_state = displayType;
  m_focusDisplay = focusDisplay;

  // Initialize new state.
  switch (displayType) {
  case DISPLAY_MENU:
    break;

  case DISPLAY_PEER_LIST:
  case DISPLAY_PEER_INFO:
  case DISPLAY_FILE_LIST:
  case DISPLAY_TRACKER_LIST:
  case DISPLAY_CHUNKS_SEEN:
  case DISPLAY_TRANSFER_LIST:
    frame->initialize_column(2);

    m_uiArray[DISPLAY_MENU]->activate(frame->frame(0), !focusDisplay);
    m_uiArray[displayType]->activate(frame->frame(1), focusDisplay);
    break;

  case DISPLAY_MAX_SIZE:
    break;
  }

  // Set title.
  switch (displayType) {
  case DISPLAY_MAX_SIZE: break;
  default: control->ui()->window_title()->set_title(m_download->download()->name()); break;
  }

  control->display()->adjust_layout();
}

void
Download::receive_next() {
  if (m_focus != m_peers.end())
    ++m_focus;
  else
    m_focus = m_peers.begin();

//   mark_dirty();
}

void
Download::receive_prev() {
  if (m_focus != m_peers.begin())
    --m_focus;
  else
    m_focus = m_peers.end();

//   mark_dirty();
}

void
Download::receive_disconnect_peer() {
  if (m_focus == m_peers.end())
    return;

  m_download->download()->disconnect_peer(*m_focus);

//   mark_dirty();
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
Download::receive_max_uploads(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_uploads_max(std::max(m_download->download()->uploads_max() + t, (uint32_t)2));
}

void
Download::receive_min_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_peers_min(std::max(m_download->download()->peers_min() + t, (uint32_t)5));
}

void
Download::receive_max_peers(int t) {
  m_windowDownloadStatus->mark_dirty();

  m_download->download()->set_peers_max(std::max(m_download->download()->peers_max() + t, (uint32_t)5));
}

void
Download::receive_snub_peer() {
  if (m_focus == m_peers.end())
    return;

  m_focus->set_snubbed(!m_focus->is_snubbed());

//   mark_dirty();
}

void
Download::receive_next_priority() {
  m_download->set_priority((m_download->priority() + 1) % 4);
}

void
Download::receive_prev_priority() {
  m_download->set_priority((m_download->priority() - 1) % 4);
}

void
Download::bind_keys() {
  m_bindings['1'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_uploads), -1);
  m_bindings['2'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_uploads), 1);
  m_bindings['3'] = sigc::bind(sigc::mem_fun(this, &Download::receive_min_peers), -5);
  m_bindings['4'] = sigc::bind(sigc::mem_fun(this, &Download::receive_min_peers), 5);
  m_bindings['5'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_peers), -5);
  m_bindings['6'] = sigc::bind(sigc::mem_fun(this, &Download::receive_max_peers), 5);
  m_bindings['+'] = sigc::mem_fun(this, &Download::receive_next_priority);
  m_bindings['-'] = sigc::mem_fun(this, &Download::receive_prev_priority);

  m_bindings['k'] = sigc::mem_fun(this, &Download::receive_disconnect_peer);

  m_bindings['t'] = sigc::bind(sigc::mem_fun(m_download->tracker_list(), &torrent::TrackerList::manual_request), false);
  m_bindings['T'] = sigc::bind(sigc::mem_fun(m_download->tracker_list(), &torrent::TrackerList::manual_request), true);

  m_bindings['p'] = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_PEER_INFO);
  m_bindings['o'] = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_TRACKER_LIST);
  m_bindings['i'] = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_CHUNKS_SEEN);
  m_bindings['u'] = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_TRANSFER_LIST);

  m_bindings[KEY_UP]   = sigc::mem_fun(this, &Download::receive_prev);
  m_bindings[KEY_DOWN] = sigc::mem_fun(this, &Download::receive_next);

  // Key bindings for sub-ui's.
  m_uiArray[DISPLAY_PEER_LIST]->bindings()[KEY_LEFT]    = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_MENU);
  m_uiArray[DISPLAY_PEER_INFO]->bindings()[KEY_LEFT]    = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_MENU);
  m_uiArray[DISPLAY_FILE_LIST]->bindings()[KEY_LEFT]    = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_MENU);
  m_uiArray[DISPLAY_TRACKER_LIST]->bindings()[KEY_LEFT] = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_MENU);
  m_uiArray[DISPLAY_CHUNKS_SEEN]->bindings()[KEY_LEFT]  = sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_MENU);
  m_uiArray[DISPLAY_TRANSFER_LIST]->bindings()[KEY_LEFT]= sigc::bind(sigc::mem_fun(this, &Download::activate_display_focus), DISPLAY_MENU);

  // Doesn't belong here.
  m_uiArray[DISPLAY_PEER_LIST]->bindings()['*'] = sigc::mem_fun(this, &Download::receive_snub_peer);
  m_uiArray[DISPLAY_PEER_INFO]->bindings()['*'] = sigc::mem_fun(this, &Download::receive_snub_peer);
}

}
