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
#include <torrent/rate.h>

#include "display/client_info.h"
#include "display/frame.h"
#include "display/manager.h"
#include "display/text_element_helpers.h"
#include "display/text_element_lambda.h"
#include "display/window_peer_list.h"
#include "input/manager.h"

#include "control.h"
#include "element_peer_list.h"
#include "element_text.h"

namespace ui {

ElementPeerList::ElementPeerList(core::Download* d) :
  m_download(d),
  m_state(DISPLAY_MAX_SIZE) {

  m_listItr = m_list.end();

  m_download->download()->peer_list(m_list);

  m_connPeerConnected    = m_download->download()->signal_peer_connected(sigc::mem_fun(*this, &ElementPeerList::receive_peer_connected));
  m_connPeerDisconnected = m_download->download()->signal_peer_disconnected(sigc::mem_fun(*this, &ElementPeerList::receive_peer_disconnected));

  m_windowList  = new display::WindowPeerList(m_download, &m_list, &m_listItr);
  m_elementInfo = create_info();

  m_elementInfo->slot_exit(sigc::bind(sigc::mem_fun(this, &ElementPeerList::activate_display), DISPLAY_LIST));

  m_bindings['k']       = sigc::mem_fun(this, &ElementPeerList::receive_disconnect_peer);
  m_bindings['*']       = sigc::mem_fun(this, &ElementPeerList::receive_snub_peer);
  m_bindings[KEY_LEFT] = m_bindings['B' - '@']  = sigc::mem_fun(&m_slotExit, &slot_type::operator());  
  m_bindings[KEY_RIGHT] = m_bindings['F' - '@'] = sigc::bind(sigc::mem_fun(this, &ElementPeerList::activate_display), DISPLAY_INFO);

  m_bindings[KEY_UP]   = m_bindings['P' - '@'] = sigc::mem_fun(this, &ElementPeerList::receive_prev);
  m_bindings[KEY_DOWN] = m_bindings['N' - '@'] = sigc::mem_fun(this, &ElementPeerList::receive_next);
}

ElementPeerList::~ElementPeerList() {
  m_connPeerConnected.disconnect();
  m_connPeerDisconnected.disconnect();

  delete m_windowList;
  delete m_elementInfo;
}

inline ElementText*
ElementPeerList::create_info() {
  using namespace display::helpers;

  ElementText* element = new ElementText(NULL);

  element->set_column(1);
  element->set_interval(1);

  // Get these bindings with some kind of string map.

  element->push_back("Peer info:");

  element->push_back("");
  element->push_column("Address:",
                       display::text_element_string_slot(rak::on(std::mem_fun(&torrent::Peer::address), std::ptr_fun(&te_address))), ":",
                       display::text_element_value_slot(rak::on(std::mem_fun(&torrent::Peer::address), std::ptr_fun(&te_port))));

  element->push_column("Id:",        display::text_element_string_slot(std::mem_fun(&torrent::Peer::id), string_base::flag_escape_html));
  element->push_column("Client:",    display::text_element_string_slot(rak::on(std::mem_fun(&torrent::Peer::id), rak::make_mem_fun(control->client_info(), &display::ClientInfo::str_str))));
  element->push_column("Options:",   display::text_element_string_slot(std::mem_fun(&torrent::Peer::options), string_base::flag_escape_hex | string_base::flag_fixed_width, 0, 8));
  element->push_column("Connected:", display::text_element_branch(std::mem_fun(&torrent::Peer::is_incoming), te_string("incoming"), te_string("outgoing")));
  element->push_column("Encrypted:", display::text_element_branch3(std::mem_fun(&torrent::Peer::is_encrypted), te_string("yes"), std::mem_fun(&torrent::Peer::is_obfuscated), te_string("handshake"), te_string("no")));

  element->push_back("");
  element->push_column("Snubbed:", display::text_element_branch(std::mem_fun(&torrent::Peer::is_snubbed), te_string("yes"), te_string("no")));
  element->push_column("Done:",    display::text_element_value_slot(rak::on(std::mem_fun(&torrent::Peer::bitfield), std::ptr_fun(&te_bitfield_percentage))));
  element->push_column("Rate:",
                       display::text_element_value_slot(rak::on(std::mem_fun(&torrent::Peer::up_rate), std::mem_fun(&torrent::Rate::rate)), value_base::flag_kb), " KB / ",
                       display::text_element_value_slot(rak::on(std::mem_fun(&torrent::Peer::down_rate), std::mem_fun(&torrent::Rate::rate)), value_base::flag_kb), " KB");
  element->push_column("Total:",
                       display::text_element_value_slot(rak::on(std::mem_fun(&torrent::Peer::up_rate), std::mem_fun(&torrent::Rate::total)), value_base::flag_xb), " / ",
                       display::text_element_value_slot(rak::on(std::mem_fun(&torrent::Peer::down_rate), std::mem_fun(&torrent::Rate::total)), value_base::flag_xb));

  element->set_column_width(element->column_width() + 1);

  return element;
}

void
ElementPeerList::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::client_error("ui::ElementPeerList::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_frame = frame;
  m_focus = focus;

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
}

void
ElementPeerList::activate_display(Display display) {
  if (display == m_state)
    return;

  switch (m_state) {
  case DISPLAY_INFO:
    m_elementInfo->disable();
    break;

  case DISPLAY_LIST:
    m_windowList->set_active(false);
    m_frame->clear();
    break;

  case DISPLAY_MAX_SIZE:
    break;
  }

  m_state = display;

  switch (m_state) {
  case DISPLAY_INFO:
    m_elementInfo->activate(m_frame, true);
    break;

  case DISPLAY_LIST:
    m_windowList->set_active(true);
    m_frame->initialize_window(m_windowList);
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

  updated_itr();
}

void
ElementPeerList::receive_prev() {
  if (m_listItr != m_list.begin())
    --m_listItr;
  else
    m_listItr = m_list.end();

  updated_itr();
}

void
ElementPeerList::receive_disconnect_peer() {
  if (m_listItr == m_list.end())
    return;

  m_download->download()->disconnect_peer(*m_listItr);
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

  updated_itr();
}

void
ElementPeerList::receive_snub_peer() {
  if (m_listItr == m_list.end())
    return;

  m_listItr->set_snubbed(!m_listItr->is_snubbed());

  updated_itr();
}

inline void
ElementPeerList::updated_itr() {
  m_windowList->mark_dirty();
  m_elementInfo->set_object(m_listItr != m_list.end() ? &*m_listItr : NULL);
}

}
