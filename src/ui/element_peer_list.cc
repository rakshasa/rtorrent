// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include <torrent/exceptions.h>
#include <torrent/rate.h>
#include <torrent/hash_string.h>
#include <torrent/peer/connection_list.h>
#include <torrent/peer/peer_info.h>

#include "display/frame.h"
#include "display/manager.h"
#include "display/text_element_string.h"
#include "display/utils.h"
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

  std::for_each(m_download->download()->connection_list()->begin(), m_download->download()->connection_list()->end(),
                rak::bind1st(std::mem_fun<void,PList,PList::const_reference>(&PList::push_back), &m_list));

  torrent::ConnectionList* connection_list = m_download->download()->connection_list();

  m_peer_connected = connection_list->signal_connected().insert(connection_list->signal_connected().end(),
                                                                tr1::bind(&ElementPeerList::receive_peer_connected, this, tr1::placeholders::_1));
  m_peer_disconnected = connection_list->signal_disconnected().insert(connection_list->signal_disconnected().end(),
                                                                      tr1::bind(&ElementPeerList::receive_peer_disconnected, this, tr1::placeholders::_1));

  m_windowList  = new display::WindowPeerList(m_download, &m_list, &m_listItr);
  m_elementInfo = create_info();

  m_elementInfo->slot_exit(std::tr1::bind(&ElementPeerList::activate_display, this, DISPLAY_LIST));

  m_bindings['k']       = std::tr1::bind(&ElementPeerList::receive_disconnect_peer, this);
  m_bindings['*']       = std::tr1::bind(&ElementPeerList::receive_snub_peer, this);
  m_bindings['B']       = std::tr1::bind(&ElementPeerList::receive_ban_peer, this);
  m_bindings[KEY_LEFT]  = m_bindings['B' - '@'] = std::tr1::bind(&slot_type::operator(), &m_slot_exit);
  m_bindings[KEY_RIGHT] = m_bindings['F' - '@'] = std::tr1::bind(&ElementPeerList::activate_display, this, DISPLAY_INFO);

  m_bindings[KEY_UP]   = m_bindings['P' - '@'] = std::tr1::bind(&ElementPeerList::receive_prev, this);
  m_bindings[KEY_DOWN] = m_bindings['N' - '@'] = std::tr1::bind(&ElementPeerList::receive_next, this);
}

ElementPeerList::~ElementPeerList() {
  torrent::ConnectionList* connection_list = m_download->download()->connection_list();

  connection_list->signal_connected().erase(m_peer_connected);
  connection_list->signal_disconnected().erase(m_peer_disconnected);

  delete m_windowList;
  delete m_elementInfo;
}

inline ElementText*
ElementPeerList::create_info() {
  using namespace display::helpers;

  ElementText* element = new ElementText(rpc::make_target());

  element->set_column(1);
  element->set_interval(1);

  element->push_back("Peer info:");

  element->push_back("");
  element->push_column("Address:",   te_command("cat=$p.address=,:,$p.port="));
  element->push_column("Id:",        te_command("p.id_html="));
  element->push_column("Client:",    te_command("p.client_version="));
  element->push_column("Options:",   te_command("p.options_str="));
  element->push_column("Connected:", te_command("if=$p.is_incoming=,incoming,outgoing"));
  element->push_column("Encrypted:", te_command("if=$p.is_encrypted=,yes,$p.is_obfuscated=,handshake,no"));

  element->push_back("");
  element->push_column("Snubbed:",   te_command("if=$p.is_snubbed=,yes,no"));
  element->push_column("Done:",      te_command("p.completed_percent="));
  element->push_column("Rate:",      te_command("cat=$convert.kb=$p.up_rate=,\\ KB\\ ,$convert.kb=$p.down_rate=,\\ KB"));
  element->push_column("Total:",     te_command("cat=$convert.kb=$p.up_total=,\\ KB\\ ,$convert.kb=$p.down_total=,\\ KB"));

  element->set_column_width(element->column_width() + 1);
  element->set_error_handler(new display::TextElementCString("No peer selected."));

  return element;
}

void
ElementPeerList::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::ElementPeerList::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_frame = frame;
  m_focus = focus;

  activate_display(DISPLAY_LIST);
}

void
ElementPeerList::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::ElementPeerList::disable(...) !is_active().");

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

  update_itr();
}

void
ElementPeerList::receive_prev() {
  if (m_listItr != m_list.begin())
    --m_listItr;
  else
    m_listItr = m_list.end();

  update_itr();
}

void
ElementPeerList::receive_disconnect_peer() {
  if (m_listItr == m_list.end())
    return;

  m_download->connection_list()->erase(*m_listItr, 0);
}

void
ElementPeerList::receive_peer_connected(torrent::Peer* p) {
  m_list.push_back(p);
}

void
ElementPeerList::receive_peer_disconnected(torrent::Peer* p) {
  PList::iterator itr = std::find(m_list.begin(), m_list.end(), p);

  if (itr == m_list.end())
    throw torrent::internal_error("ElementPeerList::receive_peer_disconnected(...) itr == m_list.end().");

  if (itr == m_listItr)
    m_listItr = m_list.erase(itr);
  else
    m_list.erase(itr);

  update_itr();
}

void
ElementPeerList::receive_snub_peer() {
  if (m_listItr == m_list.end())
    return;

  (*m_listItr)->set_snubbed(!(*m_listItr)->is_snubbed());

  update_itr();
}

void
ElementPeerList::receive_ban_peer() {
  if (m_listItr == m_list.end())
    return;

  (*m_listItr)->set_banned(true);
  m_download->download()->connection_list()->erase(*m_listItr, torrent::ConnectionList::disconnect_quick);

  update_itr();
}

void
ElementPeerList::update_itr() {
  m_windowList->mark_dirty();
  m_elementInfo->set_target(m_listItr != m_list.end() ? rpc::make_target(*m_listItr) : rpc::make_target());
}

}
