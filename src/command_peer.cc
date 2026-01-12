#include "config.h"

#include <torrent/bitfield.h>
#include <torrent/rate.h>
#include <torrent/net/socket_address.h>
#include <torrent/peer/connection_list.h>
#include <torrent/peer/peer.h>
#include <torrent/peer/peer_info.h>
#include <torrent/utils/string_manip.h>

#include "globals.h"
#include "control.h"
#include "command_helpers.h"
#include "core/manager.h"
#include "display/utils.h"

torrent::Object
retrieve_p_address(torrent::Peer* peer) {
  auto sa = peer->peer_info()->socket_address();
  auto addr_str = torrent::sa_addr_str(sa);

  if (sa->sa_family == AF_INET6)
    return "[" + addr_str + "]";

  return addr_str;
}

torrent::Object
retrieve_p_client_version(torrent::Peer* peer) {
  char buf[128];
  display::print_client_version(buf, buf + 128, peer->peer_info()->client_info());

  return std::string(buf);
}

torrent::Object
retrieve_p_completed_percent(torrent::Peer* peer) {
  return (100 * peer->bitfield()->size_set()) / peer->bitfield()->size_bits();
}

void
initialize_command_peer() {
  CMD2_PEER("p.id",                [](auto* peer, auto) { return torrent::hash_string_to_hex_str(peer->id()); });
  CMD2_PEER("p.id_html",           [](auto* peer, auto) { return torrent::hash_string_to_html_str(peer->id()); });
  CMD2_PEER("p.client_version",    [](auto* peer, auto) { return retrieve_p_client_version(peer); });

  CMD2_PEER("p.options_str",       [](auto* peer, auto) { return torrent::utils::string_from_hex_or_empty(peer->peer_info()->options(), peer->peer_info()->options() + 8); });

  CMD2_PEER("p.is_encrypted",      [](auto* peer, auto) { return peer->is_encrypted(); });
  CMD2_PEER("p.is_incoming",       [](auto* peer, auto) { return peer->is_incoming(); });
  CMD2_PEER("p.is_obfuscated",     [](auto* peer, auto) { return peer->is_obfuscated(); });
  CMD2_PEER("p.is_snubbed",        [](auto* peer, auto) { return peer->is_snubbed(); });

  CMD2_PEER("p.is_unwanted",       [](auto* peer, auto) { return peer->peer_info()->is_unwanted(); });
  CMD2_PEER("p.is_preferred",      [](auto* peer, auto) { return peer->peer_info()->is_preferred(); });

  CMD2_PEER("p.address",           [](auto* peer, auto) { return retrieve_p_address(peer); });
  CMD2_PEER("p.port",              [](auto* peer, auto) { return torrent::sa_port(peer->peer_info()->socket_address()); });

  CMD2_PEER("p.completed_percent", [](auto* peer, auto) { return retrieve_p_completed_percent(peer); });

  CMD2_PEER("p.up_rate",           [](auto* peer, auto) { return peer->up_rate()->rate(); });
  CMD2_PEER("p.up_total",          [](auto* peer, auto) { return peer->up_rate()->total(); });
  CMD2_PEER("p.down_rate",         [](auto* peer, auto) { return peer->down_rate()->rate(); });
  CMD2_PEER("p.down_total",        [](auto* peer, auto) { return peer->down_rate()->total(); });
  CMD2_PEER("p.peer_rate",         [](auto* peer, auto) { return peer->peer_rate()->rate(); });
  CMD2_PEER("p.peer_total",        [](auto* peer, auto) { return peer->peer_rate()->total(); });

  CMD2_PEER        ("p.snubbed",     [](auto* peer, auto)       { return peer->is_snubbed(); });
  CMD2_PEER_VALUE_V("p.snubbed.set", [](auto* peer, auto value) { peer->set_snubbed(value); });
  CMD2_PEER        ("p.banned",      [](auto* peer, auto)       { return peer->is_banned(); });
  CMD2_PEER_VALUE_V("p.banned.set",  [](auto* peer, auto value) { peer->set_banned(value); });

  CMD2_PEER_V("p.disconnect",         [](auto* peer, auto) { peer->disconnect(0); });
  CMD2_PEER_V("p.disconnect_delayed", [](auto* peer, auto) { peer->disconnect(torrent::ConnectionList::disconnect_delayed); });
}
