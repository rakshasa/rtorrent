// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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

#include <rak/error_number.h>
#include <rak/path.h>
#include <rak/socket_address.h>
#include <rak/string_manip.h>
#include <torrent/bitfield.h>
#include <torrent/rate.h>
#include <torrent/peer/connection_list.h>
#include <torrent/peer/peer.h>
#include <torrent/peer/peer_info.h>

#include "core/manager.h"
#include "display/utils.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
retrieve_p_id(torrent::Peer* peer) {
  const torrent::HashString* hashString = &peer->id();

  return rak::transform_hex(hashString->begin(), hashString->end());
}

torrent::Object
retrieve_p_id_html(torrent::Peer* peer) {
  const torrent::HashString* hashString = &peer->id();

  return rak::copy_escape_html(hashString->begin(), hashString->end());
}

torrent::Object
retrieve_p_address(torrent::Peer* peer) {
  return rak::socket_address::cast_from(peer->peer_info()->socket_address())->address_str();
}

torrent::Object
retrieve_p_port(torrent::Peer* peer) {
  return rak::socket_address::cast_from(peer->peer_info()->socket_address())->port();
}

torrent::Object
retrieve_p_client_version(torrent::Peer* peer) {
  char buf[128];
  display::print_client_version(buf, buf + 128, peer->peer_info()->client_info());

  return std::string(buf);
}

torrent::Object
retrieve_p_options_str(torrent::Peer* peer) {
  return rak::transform_hex(peer->peer_info()->options(), peer->peer_info()->options() + 8);
}

torrent::Object
retrieve_p_completed_percent(torrent::Peer* peer) {
  return (100 * peer->bitfield()->size_set()) / peer->bitfield()->size_bits();
}

void
initialize_command_peer() {
  CMD2_PEER("p.id",                std::tr1::bind(&retrieve_p_id, std::tr1::placeholders::_1));
  CMD2_PEER("p.id_html",           std::tr1::bind(&retrieve_p_id_html, std::tr1::placeholders::_1));
  CMD2_PEER("p.client_version",    std::tr1::bind(&retrieve_p_client_version, std::tr1::placeholders::_1));

  CMD2_PEER("p.options_str",       std::tr1::bind(&retrieve_p_options_str, std::tr1::placeholders::_1));

  CMD2_PEER("p.is_encrypted",      std::tr1::bind(&torrent::Peer::is_encrypted, std::tr1::placeholders::_1));
  CMD2_PEER("p.is_incoming",       std::tr1::bind(&torrent::Peer::is_incoming, std::tr1::placeholders::_1));
  CMD2_PEER("p.is_obfuscated",     std::tr1::bind(&torrent::Peer::is_obfuscated, std::tr1::placeholders::_1));
  CMD2_PEER("p.is_snubbed",        std::tr1::bind(&torrent::Peer::is_snubbed, std::tr1::placeholders::_1));

  CMD2_PEER("p.address",           std::tr1::bind(&retrieve_p_address, std::tr1::placeholders::_1));
  CMD2_PEER("p.port",              std::tr1::bind(&retrieve_p_port, std::tr1::placeholders::_1));

  CMD2_PEER("p.completed_percent", std::tr1::bind(&retrieve_p_completed_percent, std::tr1::placeholders::_1));

  CMD2_PEER("p.up_rate",           std::tr1::bind(&torrent::Rate::rate,  std::tr1::bind(&torrent::Peer::up_rate, std::tr1::placeholders::_1)));
  CMD2_PEER("p.up_total",          std::tr1::bind(&torrent::Rate::total, std::tr1::bind(&torrent::Peer::up_rate, std::tr1::placeholders::_1)));
  CMD2_PEER("p.down_rate",         std::tr1::bind(&torrent::Rate::rate,  std::tr1::bind(&torrent::Peer::down_rate, std::tr1::placeholders::_1)));
  CMD2_PEER("p.down_total",        std::tr1::bind(&torrent::Rate::total, std::tr1::bind(&torrent::Peer::down_rate, std::tr1::placeholders::_1)));
  CMD2_PEER("p.peer_rate",         std::tr1::bind(&torrent::Rate::rate,  std::tr1::bind(&torrent::Peer::peer_rate, std::tr1::placeholders::_1)));
  CMD2_PEER("p.peer_total",        std::tr1::bind(&torrent::Rate::total, std::tr1::bind(&torrent::Peer::peer_rate, std::tr1::placeholders::_1)));

  CMD2_PEER        ("p.snubbed",     std::tr1::bind(&torrent::Peer::is_snubbed,  std::tr1::placeholders::_1));
  CMD2_PEER_VALUE_V("p.snubbed.set", std::tr1::bind(&torrent::Peer::set_snubbed, std::tr1::placeholders::_1, std::tr1::placeholders::_2));
  CMD2_PEER        ("p.banned",      std::tr1::bind(&torrent::Peer::is_banned,   std::tr1::placeholders::_1));
  CMD2_PEER_VALUE_V("p.banned.set",  std::tr1::bind(&torrent::Peer::set_banned,  std::tr1::placeholders::_1, std::tr1::placeholders::_2));

  CMD2_PEER_V("p.disconnect",         std::tr1::bind(&torrent::Peer::disconnect, std::tr1::placeholders::_1, 0));
  CMD2_PEER_V("p.disconnect_delayed", std::tr1::bind(&torrent::Peer::disconnect, std::tr1::placeholders::_1, torrent::ConnectionList::disconnect_delayed));
}
