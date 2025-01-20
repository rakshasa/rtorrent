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

#include <functional>
#include <cstdio>
#include <unistd.h>
#include <rak/address_info.h>
#include <rak/path.h>
#include <torrent/connection_manager.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/data/file_manager.h>
#include <torrent/download/resource_manager.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "core/download.h"
#include "core/manager.h"
#include "rpc/scgi.h"
#include "ui/root.h"
#include "rpc/parse.h"
#include "rpc/parse_commands.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
apply_encryption(const torrent::Object::list_type& args) {
  uint32_t options_mask = torrent::ConnectionManager::encryption_none;

  for (torrent::Object::list_const_iterator itr = args.begin(), last = args.end(); itr != last; itr++) {
    uint32_t opt = torrent::option_find_string(torrent::OPTION_ENCRYPTION, itr->as_string().c_str());

    if (opt == torrent::ConnectionManager::encryption_none)
      options_mask = torrent::ConnectionManager::encryption_none;
    else
      options_mask |= opt;
  }

  torrent::connection_manager()->set_encryption_options(options_mask);

  return torrent::Object();
}

torrent::Object
apply_tos(const torrent::Object::string_type& arg) {
  rpc::command_base::value_type value;

  if (!rpc::parse_whole_value_nothrow(arg.c_str(), &value, 16, 1))
    value = torrent::option_find_string(torrent::OPTION_IP_TOS, arg.c_str());

  torrent::connection_manager()->set_priority(value);

  return torrent::Object();
}

torrent::Object apply_encoding_list(const std::string& arg) { torrent::encoding_list()->push_back(arg); return torrent::Object(); }

torrent::File*
rpc_find_file(core::Download* download, uint32_t index) {
  if (index >= download->file_list()->size_files())
    return NULL;

  return (*download->file_list())[index];
}

// Ergh... time to update the Tracker API to allow proper ptrs.
torrent::Tracker*
rpc_find_tracker(core::Download* download, uint32_t index) {
  if (index >= download->tracker_list()->size())
    return NULL;

  return download->tracker_list()->at(index);
}

torrent::Peer*
rpc_find_peer(core::Download* download, const torrent::HashString& hash) {
  torrent::ConnectionList::iterator itr = download->connection_list()->find(hash.c_str());

  if (itr == download->connection_list()->end())
    return NULL;

  return *itr;
}

void
initialize_rpc() {
  rpc::rpc.initialize();
  rpc::rpc.slot_find_download() = [](const char* hash) { return control->core()->download_list()->find_hex_ptr(hash); };
  rpc::rpc.slot_find_file() = [](core::Download* d, uint32_t index) { return rpc_find_file(d, index); };
  rpc::rpc.slot_find_tracker() = [](core::Download* d, uint32_t index) { return rpc_find_tracker(d, index); };
  rpc::rpc.slot_find_peer() = [](core::Download* d, const torrent::HashString& hash) { return rpc_find_peer(d, hash); };

  unsigned int count = 0;

  for (rpc::CommandMap::const_iterator itr = rpc::commands.begin(), last = rpc::commands.end(); itr != last; itr++, count++) {
    if (!(itr->second.m_flags & rpc::CommandMap::flag_public_rpc))
      continue;

    rpc::rpc.insert_command(itr->first.c_str(), itr->second.m_parm, itr->second.m_doc);
  }

  lt_log_print(torrent::LOG_RPC_EVENTS, "RPC initialized with %u functions.", count);
}

torrent::Object
apply_scgi(const std::string& arg, int type) {
  if (worker_thread->scgi() != NULL)
    throw torrent::input_error("SCGI already enabled.");

  initialize_rpc();

  rpc::SCgi* scgi = new rpc::SCgi;

  rak::address_info* ai = NULL;
  rak::socket_address sa;
  rak::socket_address* saPtr;

  try {
    int port, err;
    char dummy;
    char address[1024];
    std::string path;

    switch (type) {
    case 1:
      if (std::sscanf(arg.c_str(), ":%i%c", &port, &dummy) == 1) {
        sa.sa_inet()->clear();
        saPtr = &sa;

        lt_log_print(torrent::LOG_RPC_EVENTS, "SCGI socket is open to any address and is a security risk");

      } else if (std::sscanf(arg.c_str(), "%1023[^:]:%i%c", address, &port, &dummy) == 2 ||
                 std::sscanf(arg.c_str(), "[%64[^]]]:%i%c", address, &port, &dummy) == 2) { // [xx::xx]:port format
        if ((err = rak::address_info::get_address_info(address,PF_UNSPEC, SOCK_STREAM, &ai)) != 0)
          throw torrent::input_error("Could not bind address: " + std::string(rak::address_info::strerror(err)) + ".");

        saPtr = ai->address();

        lt_log_print(torrent::LOG_RPC_EVENTS, "SCGI socket is bound to an address and might be a security risk");

      } else {
        throw torrent::input_error("Could not parse address.");
      }

      if (port <= 0 || port >= (1 << 16))
        throw torrent::input_error("Invalid port number.");

      saPtr->set_port(port);
      scgi->open_port(saPtr, saPtr->length(), rpc::call_command_value("network.scgi.dont_route"));

      break;

    case 2:
    default:
      path = rak::path_expand(arg);

      unlink(path.c_str());
      scgi->open_named(path);
      break;
    }

    if (ai != NULL) rak::address_info::free_address_info(ai);

  } catch (torrent::local_error& e) {
    if (ai != NULL) rak::address_info::free_address_info(ai);

    delete scgi;
    throw torrent::input_error(e.what());
  }

  worker_thread->set_scgi(scgi);
  return torrent::Object();
}

torrent::Object
apply_xmlrpc_dialect(const std::string& arg) {
  int value;

  if (arg == "i8")
    value = rpc::XmlRpc::dialect_i8;
  else if (arg == "apache")
    value = rpc::XmlRpc::dialect_apache;
  else if (arg == "generic")
    value = rpc::XmlRpc::dialect_generic;
  else
    value = -1;

  rpc::rpc.set_dialect(value);
  return torrent::Object();
}

void
initialize_command_network() {
  torrent::ConnectionManager* cm = torrent::connection_manager();
  torrent::FileManager* fileManager = torrent::file_manager();
  core::CurlStack* httpStack = control->core()->http_stack();

  CMD2_ANY_STRING  ("encoding.add", std::bind(&apply_encoding_list, std::placeholders::_2));

  // Isn't port_open used?
  CMD2_VAR_BOOL    ("network.port_open",   true);
  CMD2_VAR_BOOL    ("network.port_random", true);
  CMD2_VAR_STRING  ("network.port_range",  "6881-6999");

  CMD2_ANY         ("network.listen.port",        std::bind(&torrent::ConnectionManager::listen_port, cm));
  CMD2_ANY         ("network.listen.backlog",     std::bind(&torrent::ConnectionManager::listen_backlog, cm));
  CMD2_ANY_VALUE_V ("network.listen.backlog.set", std::bind(&torrent::ConnectionManager::set_listen_backlog, cm, std::placeholders::_2));

  CMD2_VAR_BOOL    ("protocol.pex",            true);
  CMD2_ANY_LIST    ("protocol.encryption.set", std::bind(&apply_encryption, std::placeholders::_2));

  CMD2_VAR_STRING  ("protocol.connection.leech", "leech");
  CMD2_VAR_STRING  ("protocol.connection.seed",  "seed");

  CMD2_VAR_STRING  ("protocol.choke_heuristics.up.leech", "upload_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.up.seed",  "upload_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.down.leech", "download_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.down.seed",  "download_leech");

  CMD2_ANY         ("network.http.cacert",                std::bind(&core::CurlStack::http_cacert, httpStack));
  CMD2_ANY_STRING_V("network.http.cacert.set",            std::bind(&core::CurlStack::set_http_cacert, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.capath",                std::bind(&core::CurlStack::http_capath, httpStack));
  CMD2_ANY_STRING_V("network.http.capath.set",            std::bind(&core::CurlStack::set_http_capath, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.dns_cache_timeout",     std::bind(&core::CurlStack::dns_timeout, httpStack));
  CMD2_ANY_VALUE_V ("network.http.dns_cache_timeout.set", std::bind(&core::CurlStack::set_dns_timeout, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.current_open",          std::bind(&core::CurlStack::active, httpStack));
  CMD2_ANY         ("network.http.max_open",              std::bind(&core::CurlStack::max_active, httpStack));
  CMD2_ANY_VALUE_V ("network.http.max_open.set",          std::bind(&core::CurlStack::set_max_active, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.proxy_address",         std::bind(&core::CurlStack::http_proxy, httpStack));
  CMD2_ANY_STRING_V("network.http.proxy_address.set",     std::bind(&core::CurlStack::set_http_proxy, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.ssl_verify_host",       std::bind(&core::CurlStack::ssl_verify_host, httpStack));
  CMD2_ANY_VALUE_V ("network.http.ssl_verify_host.set",   std::bind(&core::CurlStack::set_ssl_verify_host, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.ssl_verify_peer",       std::bind(&core::CurlStack::ssl_verify_peer, httpStack));
  CMD2_ANY_VALUE_V ("network.http.ssl_verify_peer.set",   std::bind(&core::CurlStack::set_ssl_verify_peer, httpStack, std::placeholders::_2));

  CMD2_ANY         ("network.send_buffer.size",        std::bind(&torrent::ConnectionManager::send_buffer_size, cm));
  CMD2_ANY_VALUE_V ("network.send_buffer.size.set",    std::bind(&torrent::ConnectionManager::set_send_buffer_size, cm, std::placeholders::_2));
  CMD2_ANY         ("network.receive_buffer.size",     std::bind(&torrent::ConnectionManager::receive_buffer_size, cm));
  CMD2_ANY_VALUE_V ("network.receive_buffer.size.set", std::bind(&torrent::ConnectionManager::set_receive_buffer_size, cm, std::placeholders::_2));
  CMD2_ANY_STRING  ("network.tos.set",                 std::bind(&apply_tos, std::placeholders::_2));

  CMD2_ANY         ("network.bind_address",          std::bind(&core::Manager::bind_address, control->core()));
  CMD2_ANY_STRING_V("network.bind_address.set",      std::bind(&core::Manager::set_bind_address, control->core(), std::placeholders::_2));
  CMD2_ANY         ("network.local_address",         std::bind(&core::Manager::local_address, control->core()));
  CMD2_ANY_STRING_V("network.local_address.set",     std::bind(&core::Manager::set_local_address, control->core(), std::placeholders::_2));
  CMD2_ANY         ("network.proxy_address",         std::bind(&core::Manager::proxy_address, control->core()));
  CMD2_ANY_STRING_V("network.proxy_address.set",     std::bind(&core::Manager::set_proxy_address, control->core(), std::placeholders::_2));

  CMD2_ANY         ("network.open_files",            std::bind(&torrent::FileManager::open_files, fileManager));
  CMD2_ANY         ("network.max_open_files",        std::bind(&torrent::FileManager::max_open_files, fileManager));
  CMD2_ANY_VALUE_V ("network.max_open_files.set",    std::bind(&torrent::FileManager::set_max_open_files, fileManager, std::placeholders::_2));
  CMD2_ANY         ("network.total_handshakes",      std::bind(&torrent::total_handshakes));
  CMD2_ANY         ("network.open_sockets",          std::bind(&torrent::ConnectionManager::size, cm));
  CMD2_ANY         ("network.max_open_sockets",      std::bind(&torrent::ConnectionManager::max_size, cm));
  CMD2_ANY_VALUE_V ("network.max_open_sockets.set",  std::bind(&torrent::ConnectionManager::set_max_size, cm, std::placeholders::_2));

  CMD2_ANY_STRING  ("network.scgi.open_port",        std::bind(&apply_scgi, std::placeholders::_2, 1));
  CMD2_ANY_STRING  ("network.scgi.open_local",       std::bind(&apply_scgi, std::placeholders::_2, 2));
  CMD2_VAR_BOOL    ("network.scgi.dont_route",       false);

  CMD2_ANY_STRING  ("network.xmlrpc.dialect.set",    [](const auto&, const auto& arg) { return apply_xmlrpc_dialect(arg); })
  CMD2_ANY         ("network.xmlrpc.size_limit",     [](const auto&, const auto&){ return rpc::rpc.size_limit(); });
  CMD2_ANY_VALUE_V ("network.xmlrpc.size_limit.set", [](const auto&, const auto& arg){ return rpc::rpc.set_size_limit(arg); });

  CMD2_VAR_BOOL    ("network.rpc.use_xmlrpc",        true);
  CMD2_VAR_BOOL    ("network.rpc.use_jsonrpc",       true);

  CMD2_ANY         ("network.block.ipv4",            std::bind(&torrent::ConnectionManager::is_block_ipv4, cm));
  CMD2_ANY_VALUE_V ("network.block.ipv4.set",        std::bind(&torrent::ConnectionManager::set_block_ipv4, cm, std::placeholders::_2));
  CMD2_ANY         ("network.block.ipv6",            std::bind(&torrent::ConnectionManager::is_block_ipv6, cm));
  CMD2_ANY_VALUE_V ("network.block.ipv6.set",        std::bind(&torrent::ConnectionManager::set_block_ipv6, cm, std::placeholders::_2));

  CMD2_ANY         ("network.prefer.ipv6",           std::bind(&torrent::ConnectionManager::is_prefer_ipv6, cm));
  CMD2_ANY_VALUE_V ("network.prefer.ipv6.set",       std::bind(&torrent::ConnectionManager::set_prefer_ipv6, cm, std::placeholders::_2));
}
