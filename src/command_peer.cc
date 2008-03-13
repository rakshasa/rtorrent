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

#define ADD_CP_SLOT(key, function, slot, parm, doc)    \
  commandPeerSlotsItr->set_slot(slot); \
  rpc::commands.insert_type(key, commandPeerSlotsItr++, &rpc::CommandSlot<torrent::Peer*>::function, rpc::CommandMap::flag_dont_delete, parm, doc);

#define ADD_CP_SLOT_PUBLIC(key, function, slot, parm, doc)    \
  commandPeerSlotsItr->set_slot(slot); \
  rpc::commands.insert_type(key, commandPeerSlotsItr++, &rpc::CommandSlot<torrent::Peer*>::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_CP_VOID(key, slot) \
  ADD_CP_SLOT_PUBLIC("p.get_" key, call_unknown, rpc::object_fn(slot), "i:", "")

#define ADD_CP_VALUE(key, get) \
  ADD_CP_SLOT_PUBLIC("p." key, call_unknown, rpc::object_void_fn<torrent::Peer*>(get), "i:", "")

#define ADD_CP_VALUE_UNI(key, get) \
  ADD_CP_SLOT_PUBLIC("p.get_" key, call_unknown, rpc::object_void_fn<torrent::Peer*>(get), "i:", "")

#define ADD_CP_VALUE_BI(key, set, get) \
  ADD_CP_SLOT_PUBLIC("p.set_" key, call_value, rpc::object_value_fn<torrent::Peer*>(set), "i:i", "") \
  ADD_CP_SLOT_PUBLIC("p.get_" key, call_unknown, rpc::object_void_fn<torrent::Peer*>(get), "i:", "")

#define ADD_CP_VALUE_MEM_UNI(key, target, get) \
  ADD_CP_SLOT_PUBLIC("p.get_" key, call_unknown, rpc::object_void_fn<torrent::Peer*>(rak::on(std::mem_fun(target), std::mem_fun(get))), "i:", "");

#define ADD_CP_STRING_UNI(key, get) \
  ADD_CP_SLOT_PUBLIC("p.get_" key, call_unknown, rpc::object_void_fn<torrent::Peer*>(get), "s:", "")

void
initialize_command_peer() {
  ADD_CP_STRING_UNI("id",                 std::ptr_fun(&retrieve_p_id));
  ADD_CP_STRING_UNI("id_html",            std::ptr_fun(&retrieve_p_id_html));
  ADD_CP_STRING_UNI("client_version",     std::ptr_fun(&retrieve_p_client_version));

  ADD_CP_STRING_UNI("options_str",        std::ptr_fun(&retrieve_p_options_str));

  ADD_CP_VALUE("is_encrypted",            std::mem_fun(&torrent::Peer::is_encrypted));
  ADD_CP_VALUE("is_incoming",             std::mem_fun(&torrent::Peer::is_incoming));
  ADD_CP_VALUE("is_obfuscated",           std::mem_fun(&torrent::Peer::is_obfuscated));
  ADD_CP_VALUE("is_snubbed",              std::mem_fun(&torrent::Peer::is_snubbed));

  ADD_CP_STRING_UNI("address",            std::ptr_fun(&retrieve_p_address));
  ADD_CP_VALUE_UNI("port",                std::ptr_fun(&retrieve_p_port));

  ADD_CP_VALUE_UNI("completed_percent",   std::ptr_fun(&retrieve_p_completed_percent));

  ADD_CP_VALUE_MEM_UNI("up_rate",         &torrent::Peer::up_rate, &torrent::Rate::rate);
  ADD_CP_VALUE_MEM_UNI("up_total",        &torrent::Peer::up_rate, &torrent::Rate::total);
  ADD_CP_VALUE_MEM_UNI("down_rate",       &torrent::Peer::down_rate, &torrent::Rate::rate);
  ADD_CP_VALUE_MEM_UNI("down_total",      &torrent::Peer::down_rate, &torrent::Rate::total);
  ADD_CP_VALUE_MEM_UNI("peer_rate",       &torrent::Peer::peer_rate, &torrent::Rate::rate);
  ADD_CP_VALUE_MEM_UNI("peer_total",      &torrent::Peer::peer_rate, &torrent::Rate::total);
}
