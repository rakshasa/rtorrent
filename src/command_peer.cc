#include "config.h"

#include <torrent/bitfield.h>
#include <torrent/rate.h>
#include <torrent/net/socket_address.h>
#include <torrent/peer/connection_list.h>
#include <torrent/peer/peer.h>
#include <torrent/peer/peer_info.h>
#include <torrent/utils/string_manip.h>

#include "globals.h"
#include "command_helpers.h"
#include "control.h"
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
retrieve_p_options_str(torrent::Peer* peer) {
  return torrent::utils::transform_to_hex_str(peer->peer_info()->options(), peer->peer_info()->options() + 8);
}

torrent::Object
retrieve_p_completed_percent(torrent::Peer* peer) {
  if (peer->bitfield()->size_bits() == 0)
    return int64_t(0);
  return (100 * peer->bitfield()->size_set()) / peer->bitfield()->size_bits();
}

void
initialize_command_peer() {
  CMD2_PEER("p.id",                [](auto* peer, auto) { return torrent::utils::transform_to_hex_str(peer->id()); });
  CMD2_PEER("p.id_html",           [](auto* peer, auto) { return torrent::utils::copy_escape_html_str(peer->id()); });
  CMD2_PEER("p.client_version",    std::bind(&retrieve_p_client_version, std::placeholders::_1));

  CMD2_PEER("p.options_str",       std::bind(&retrieve_p_options_str, std::placeholders::_1));

  CMD2_PEER("p.is_encrypted",      std::bind(&torrent::Peer::is_encrypted, std::placeholders::_1));
  CMD2_PEER("p.is_incoming",       std::bind(&torrent::Peer::is_incoming, std::placeholders::_1));
  CMD2_PEER("p.is_obfuscated",     std::bind(&torrent::Peer::is_obfuscated, std::placeholders::_1));
  CMD2_PEER("p.is_snubbed",        std::bind(&torrent::Peer::is_snubbed, std::placeholders::_1));

  CMD2_PEER("p.is_unwanted",       std::bind(&torrent::PeerInfo::is_unwanted,  std::bind(&torrent::Peer::peer_info, std::placeholders::_1)));
  CMD2_PEER("p.is_preferred",      std::bind(&torrent::PeerInfo::is_preferred, std::bind(&torrent::Peer::peer_info, std::placeholders::_1)));

  CMD2_PEER("p.address",           std::bind(&retrieve_p_address, std::placeholders::_1));
  CMD2_PEER("p.port",              [](auto* peer, auto) { return torrent::sa_port(peer->peer_info()->socket_address()); });

  CMD2_PEER("p.completed_percent", std::bind(&retrieve_p_completed_percent, std::placeholders::_1));

  CMD2_PEER("p.up_rate",           std::bind(&torrent::Rate::rate,  std::bind(&torrent::Peer::up_rate, std::placeholders::_1)));
  CMD2_PEER("p.up_total",          std::bind(&torrent::Rate::total, std::bind(&torrent::Peer::up_rate, std::placeholders::_1)));
  CMD2_PEER("p.down_rate",         std::bind(&torrent::Rate::rate,  std::bind(&torrent::Peer::down_rate, std::placeholders::_1)));
  CMD2_PEER("p.down_total",        std::bind(&torrent::Rate::total, std::bind(&torrent::Peer::down_rate, std::placeholders::_1)));
  CMD2_PEER("p.peer_rate",         std::bind(&torrent::Rate::rate,  std::bind(&torrent::Peer::peer_rate, std::placeholders::_1)));
  CMD2_PEER("p.peer_total",        std::bind(&torrent::Rate::total, std::bind(&torrent::Peer::peer_rate, std::placeholders::_1)));

  CMD2_PEER        ("p.snubbed",     std::bind(&torrent::Peer::is_snubbed,  std::placeholders::_1));
  CMD2_PEER_VALUE_V("p.snubbed.set", std::bind(&torrent::Peer::set_snubbed, std::placeholders::_1, std::placeholders::_2));
  CMD2_PEER        ("p.banned",      std::bind(&torrent::Peer::is_banned,   std::placeholders::_1));
  CMD2_PEER_VALUE_V("p.banned.set",  std::bind(&torrent::Peer::set_banned,  std::placeholders::_1, std::placeholders::_2));

  CMD2_PEER_V("p.disconnect",         std::bind(&torrent::Peer::disconnect, std::placeholders::_1, 0));
  CMD2_PEER_V("p.disconnect_delayed", std::bind(&torrent::Peer::disconnect, std::placeholders::_1, torrent::ConnectionList::disconnect_delayed));

  rpc::rpc.mark_safe("p.address");
  rpc::rpc.mark_safe("p.port");
  rpc::rpc.mark_safe("p.client_version");
  rpc::rpc.mark_safe("p.options_str");
  rpc::rpc.mark_safe("p.id");
  rpc::rpc.mark_safe("p.id_html");
  rpc::rpc.mark_safe("p.up_rate");
  rpc::rpc.mark_safe("p.up_total");
  rpc::rpc.mark_safe("p.down_rate");
  rpc::rpc.mark_safe("p.down_total");
  rpc::rpc.mark_safe("p.peer_rate");
  rpc::rpc.mark_safe("p.peer_total");
  rpc::rpc.mark_safe("p.is_encrypted");
  rpc::rpc.mark_safe("p.is_incoming");
  rpc::rpc.mark_safe("p.is_obfuscated");
  rpc::rpc.mark_safe("p.is_snubbed");
  rpc::rpc.mark_safe("p.is_unwanted");
  rpc::rpc.mark_safe("p.is_preferred");
  rpc::rpc.mark_safe("p.snubbed");
  rpc::rpc.mark_safe("p.snubbed.set");
  rpc::rpc.mark_safe("p.banned");
  rpc::rpc.mark_safe("p.banned.set");
  rpc::rpc.mark_safe("p.completed_percent");
  rpc::rpc.mark_safe("p.disconnect");
  rpc::rpc.mark_safe("p.disconnect_delayed");
}
