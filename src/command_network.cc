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

#include <functional>
#include <fstream>
#include <cstdio>
#include <rak/address_info.h>
#include <rak/path.h>
#include <torrent/connection_manager.h>
#include <torrent/dht_manager.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/data/file_manager.h>
#include <torrent/download/resource_manager.h>
#include <torrent/peer/peer_list.h>
#include <torrent/utils/option_strings.h>

#include "core/dht_manager.h"
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

torrent::Object apply_hash_read_ahead(int arg)              { torrent::set_hash_read_ahead(arg << 20); return torrent::Object(); }
torrent::Object apply_hash_interval(int arg)                { torrent::set_hash_interval(arg * 1000); return torrent::Object(); }
torrent::Object apply_encoding_list(const std::string& arg) { torrent::encoding_list()->push_back(arg); return torrent::Object(); }

struct call_add_node_t {
  call_add_node_t(int port) : m_port(port) { }

  void operator() (const sockaddr* sa, int err) {
    if (sa == NULL)
      control->core()->push_log("Could not resolve host.");
    else
      torrent::dht_manager()->add_node(sa, m_port);
  }

  int m_port;
};

torrent::Object
apply_dht_add_node(const std::string& arg) {
  if (!torrent::dht_manager()->is_valid())
    throw torrent::input_error("DHT not enabled.");

  int port, ret;
  char dummy;
  char host[1024];

  ret = std::sscanf(arg.c_str(), "%1023[^:]:%i%c", host, &port, &dummy);

  if (ret == 1)
    port = 6881;
  else if (ret != 2)
    throw torrent::input_error("Could not parse host.");

  if (port < 1 || port > 65535)
    throw torrent::input_error("Invalid port number.");

  torrent::connection_manager()->resolver()(host, (int)rak::socket_address::pf_inet, SOCK_DGRAM, call_add_node_t(port));
  return torrent::Object();
}

torrent::Object
apply_enable_trackers(int64_t arg) {
  for (core::Manager::DListItr itr = control->core()->download_list()->begin(), last = control->core()->download_list()->end(); itr != last; ++itr) {
    std::for_each((*itr)->tracker_list()->begin(), (*itr)->tracker_list()->end(),
                  arg ? std::mem_fun(&torrent::Tracker::enable) : std::mem_fun(&torrent::Tracker::disable));

    if (arg && !rpc::call_command_value("trackers.use_udp"))
      (*itr)->enable_udp_trackers(false);
  }    

  return torrent::Object();
}

torrent::File*
xmlrpc_find_file(core::Download* download, uint32_t index) {
  if (index >= download->file_list()->size_files())
    return NULL;

  return (*download->file_list())[index];
}

// Ergh... time to update the Tracker API to allow proper ptrs.
torrent::Tracker*
xmlrpc_find_tracker(core::Download* download, uint32_t index) {
  if (index >= download->tracker_list()->size())
    return NULL;

  return download->tracker_list()->at(index);
}

torrent::Peer*
xmlrpc_find_peer(core::Download* download, const torrent::HashString& hash) {
  torrent::ConnectionList::iterator itr = download->connection_list()->find(hash.c_str());

  if (itr == download->connection_list()->end())
    return NULL;

  return *itr;
}

void
initialize_xmlrpc() {
  rpc::xmlrpc.initialize();
  rpc::xmlrpc.set_slot_find_download(rak::mem_fn(control->core()->download_list(), &core::DownloadList::find_hex_ptr));
  rpc::xmlrpc.set_slot_find_file(rak::ptr_fn(&xmlrpc_find_file));
  rpc::xmlrpc.set_slot_find_tracker(rak::ptr_fn(&xmlrpc_find_tracker));
  rpc::xmlrpc.set_slot_find_peer(rak::ptr_fn(&xmlrpc_find_peer));

  unsigned int count = 0;

  for (rpc::CommandMap::const_iterator itr = rpc::commands.begin(), last = rpc::commands.end(); itr != last; itr++, count++) {
    if (!(itr->second.m_flags & rpc::CommandMap::flag_public_xmlrpc))
      continue;

    rpc::xmlrpc.insert_command(itr->first, itr->second.m_parm, itr->second.m_doc);
  }

  char buffer[128];
  sprintf(buffer, "XMLRPC initialized with %u functions.", count);

  control->core()->push_log(buffer);
}

torrent::Object
apply_scgi(const std::string& arg, int type) {
  if (worker_thread->scgi() != NULL)
    throw torrent::input_error("SCGI already enabled.");

  if (!rpc::xmlrpc.is_valid())
    initialize_xmlrpc();

  rpc::SCgi* scgi = new rpc::SCgi;

  rak::address_info* ai = NULL;
  rak::socket_address sa;
  rak::socket_address* saPtr;

  try {
    int port, err;
    char dummy;
    char address[1024];

    switch (type) {
    case 1:
      if (std::sscanf(arg.c_str(), ":%i%c", &port, &dummy) == 1) {
        sa.sa_inet()->clear();
        saPtr = &sa;

        control->core()->push_log("The SCGI socket has not been bound to any address and likely poses a security risk.");

      } else if (std::sscanf(arg.c_str(), "%1023[^:]:%i%c", address, &port, &dummy) == 2) {
        if ((err = rak::address_info::get_address_info(address, PF_INET, SOCK_STREAM, &ai)) != 0)
          throw torrent::input_error("Could not bind address: " + std::string(rak::address_info::strerror(err)) + ".");

        saPtr = ai->address();

        control->core()->push_log("The SCGI socket is bound to a specific network device yet may still pose a security risk, consider using 'scgi_local'.");

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
      scgi->open_named(rak::path_expand(arg));
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

  rpc::xmlrpc.set_dialect(value);
  return torrent::Object();
}

//
// IP filter stuff:
//

void
ipv4_filter_parse(const char* address, int value) {
  uint32_t ip_values[4] = { 0, 0, 0, 0 };
  unsigned int block = rpc::ipv4_table::mask_bits;

  char ip_dot;
  int values_read;

  if ((values_read = sscanf(address, "%u%1[.]%u%1[.]%u%1[.]%u/%u",
                            ip_values + 0, &ip_dot,
                            ip_values + 1, &ip_dot,
                            ip_values + 2, &ip_dot,
                            ip_values + 3, 
                            &block)) < 2 ||

      // Make sure the dot is included.
      (values_read < 7 && values_read % 2) ||

      ip_values[0] >= 256 ||
      ip_values[1] >= 256 ||
      ip_values[2] >= 256 ||
      ip_values[3] >= 256 ||
       
      block > rpc::ipv4_table::mask_bits)
    throw torrent::input_error("Invalid address format.");

  // E.g. '10.10.' will be '10.10.0.0/16'.
  if (values_read < 7)
    block = 8 * (values_read / 2);

  torrent::PeerList::ipv4_filter()->insert((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3],
                                           rpc::ipv4_table::mask_bits - block, value);
}

torrent::Object
apply_ip_tables_insert_table(const std::string& args) {
  if (ip_tables.find(args) != ip_tables.end())
    throw torrent::input_error("IP table already exists.");

  ip_tables.insert(args);
  return torrent::Object();
}

torrent::Object
apply_ip_tables_get(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  const std::string& name    = (args_itr++)->as_string();
  const std::string& address = (args_itr++)->as_string();

  // Move to a helper function, add support for addresses.
  uint32_t ip_values[4];

  if (sscanf(address.c_str(), "%u.%u.%u.%u",
             ip_values + 0, ip_values + 1, ip_values + 2, ip_values + 3) != 4)
    throw torrent::input_error("Invalid address format.");

  rpc::ip_table_list::iterator table_itr = ip_tables.find(name);

  if (table_itr == ip_tables.end())
    throw torrent::input_error("Could not find ip table.");

  return table_itr->table.at((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3]);
}

torrent::Object
apply_ip_tables_add_address(const torrent::Object::list_type& args) {
  if (args.size() != 3)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  const std::string& name      = (args_itr++)->as_string();
  const std::string& address   = (args_itr++)->as_string();
  const std::string& value_str = (args_itr++)->as_string();
  
  // Move to a helper function, add support for addresses.
  uint32_t ip_values[4];
  unsigned int block = rpc::ipv4_table::mask_bits;

  if (sscanf(address.c_str(), "%u.%u.%u.%u/%u",
             ip_values + 0, ip_values + 1, ip_values + 2, ip_values + 3, &block) < 4 ||
      block > rpc::ipv4_table::mask_bits)
    throw torrent::input_error("Invalid address format.");

  int value;

  if (value_str == "block")
    value = 1;
  else
    throw torrent::input_error("Invalid value.");

  rpc::ip_table_list::iterator table_itr = ip_tables.find(name);

  if (table_itr == ip_tables.end())
    throw torrent::input_error("Could not find ip table.");

  table_itr->table.insert((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3],
                          rpc::ipv4_table::mask_bits - block, value);

  return torrent::Object();
}

//
// IPv4 filter functions:
//

torrent::Object
apply_ipv4_filter_size_data() {
  return torrent::PeerList::ipv4_filter()->sizeof_data();
}

torrent::Object
apply_ipv4_filter_get(const std::string& args) {
  // Move to a helper function, add support for addresses.
  uint32_t ip_values[4];

  if (sscanf(args.c_str(), "%u.%u.%u.%u",
             ip_values + 0, ip_values + 1, ip_values + 2, ip_values + 3) != 4)
    throw torrent::input_error("Invalid address format.");

  return torrent::PeerList::ipv4_filter()->at((ip_values[0] << 24) + (ip_values[1] << 16) + (ip_values[2] << 8) + ip_values[3]);
}

torrent::Object
apply_ipv4_filter_add_address(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  ipv4_filter_parse(args.front().as_string().c_str(),
                    torrent::option_find_string(torrent::OPTION_IP_FILTER, args.back().as_string().c_str()));
  return torrent::Object();
}

torrent::Object
apply_ipv4_filter_load(const torrent::Object::list_type& args) {
  if (args.size() != 2)
    throw torrent::input_error("Incorrect number of arguments.");

  torrent::Object::list_const_iterator args_itr = args.begin();

  std::fstream file(rak::path_expand(args.front().as_string()).c_str(), std::ios::in);
  
  if (!file.is_open())
    throw torrent::input_error("Could not open ip filter file: " + args.front().as_string());

  int value = torrent::option_find_string(torrent::OPTION_IP_FILTER, args.back().as_string().c_str());

  char buffer[4096];
  unsigned int lineNumber = 0;

  try {
    while (file.good() && !file.getline(buffer, 4096).fail()) {
      if (file.gcount() == 0)
        throw torrent::internal_error("parse_command_file(...) file.gcount() == 0.");

      int lineLength = file.gcount() - 1;
      // In case we are at the end of the file and the last character is
      // not a line feed, we'll just increase the read character count so 
      // that the last would also be included in option line.
      if (file.eof() && file.get() != '\n')
        lineLength++;
      
      lineNumber++;

      if (buffer[0] == '\0' || buffer[0] == '#')
        continue;

      ipv4_filter_parse(buffer, value);
    }

  } catch (torrent::input_error& e) {
    snprintf(buffer, 2048, "Error in ip filter file: %s:%u: %s", args.front().as_string().c_str(), lineNumber, e.what());

    throw torrent::input_error(buffer);
  }

  snprintf(buffer, 2048, "Loaded %u %s address blocks (%u kb in-memory) from '%s'.",
           lineNumber,
           args.back().as_string().c_str(),
           torrent::PeerList::ipv4_filter()->sizeof_data() / 1024,
           args.front().as_string().c_str());
  control->core()->push_log(buffer);

  return torrent::Object();
}

void
initialize_command_network() {
  torrent::ConnectionManager* cm = torrent::connection_manager();
  torrent::FileManager* fileManager = torrent::file_manager();
  core::CurlStack* httpStack = control->core()->http_stack();

  CMD2_ANY         ("strings.connection_type", std::bind(&torrent::option_list_strings, torrent::OPTION_CONNECTION_TYPE));
  CMD2_ANY         ("strings.encryption",      std::bind(&torrent::option_list_strings, torrent::OPTION_ENCRYPTION));
  CMD2_ANY         ("strings.ip_filter",       std::bind(&torrent::option_list_strings, torrent::OPTION_IP_FILTER));
  CMD2_ANY         ("strings.ip_tos",          std::bind(&torrent::option_list_strings, torrent::OPTION_IP_TOS));

  CMD2_VAR_BOOL    ("log.handshake", false);
  CMD2_VAR_STRING  ("log.tracker",   "");

  // CMD2_ANY_STRING  ("encoding_list",    std::bind(&apply_encoding_list, std::placeholders::_2));
  CMD2_ANY_STRING  ("encoding.add", std::bind(&apply_encoding_list, std::placeholders::_2));

  // Isn't port_open used?
  CMD2_VAR_BOOL    ("network.port_open",   true);
  CMD2_VAR_BOOL    ("network.port_random", true);
  CMD2_VAR_STRING  ("network.port_range",  "6881-6999");

  CMD2_VAR_BOOL    ("protocol.pex",            true);
  CMD2_ANY_LIST    ("protocol.encryption.set", std::bind(&apply_encryption, std::placeholders::_2));

  CMD2_VAR_STRING  ("protocol.connection.leech", "leech");
  CMD2_VAR_STRING  ("protocol.connection.seed",  "seed");

  CMD2_VAR_STRING  ("protocol.choke_heuristics.up.leech", "upload_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.up.seed",  "upload_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.down.leech", "download_leech");
  CMD2_VAR_STRING  ("protocol.choke_heuristics.down.seed",  "download_leech");

  CMD2_ANY         ("network.http.capath",                std::bind(&core::CurlStack::http_capath, httpStack));
  CMD2_ANY_STRING_V("network.http.capath.set",            std::bind(&core::CurlStack::set_http_capath, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.cacert",                std::bind(&core::CurlStack::http_cacert, httpStack));
  CMD2_ANY_STRING_V("network.http.cacert.set",            std::bind(&core::CurlStack::set_http_cacert, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.proxy_address",         std::bind(&core::CurlStack::http_proxy, httpStack));
  CMD2_ANY_STRING_V("network.http.proxy_address.set",     std::bind(&core::CurlStack::set_http_proxy, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.max_open",              std::bind(&core::CurlStack::max_active, httpStack));
  CMD2_ANY_VALUE_V ("network.http.max_open.set",          std::bind(&core::CurlStack::set_max_active, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.ssl_verify_peer",       std::bind(&core::CurlStack::ssl_verify_peer, httpStack));
  CMD2_ANY_VALUE_V ("network.http.ssl_verify_peer.set",   std::bind(&core::CurlStack::set_ssl_verify_peer, httpStack, std::placeholders::_2));
  CMD2_ANY         ("network.http.dns_cache_timeout",     std::bind(&core::CurlStack::dns_timeout, httpStack));
  CMD2_ANY_VALUE_V ("network.http.dns_cache_timeout.set", std::bind(&core::CurlStack::set_dns_timeout, httpStack, std::placeholders::_2));

  CMD2_ANY         ("network.send_buffer.size",        std::bind(&torrent::ConnectionManager::send_buffer_size, cm));
  CMD2_ANY_VALUE_V ("network.send_buffer.size.set",    std::bind(&torrent::ConnectionManager::set_send_buffer_size, cm, std::placeholders::_2));
  CMD2_ANY         ("network.receive_buffer.size",     std::bind(&torrent::ConnectionManager::receive_buffer_size, cm));
  CMD2_ANY_VALUE_V ("network.receive_buffer.size.set", std::bind(&torrent::ConnectionManager::set_receive_buffer_size, cm, std::placeholders::_2));
  CMD2_ANY_STRING  ("network.tos.set",                 std::bind(&apply_tos, std::placeholders::_2));

  CMD2_ANY         ("network.bind_address",        std::bind(&core::Manager::bind_address, control->core()));
  CMD2_ANY_STRING_V("network.bind_address.set",    std::bind(&core::Manager::set_bind_address, control->core(), std::placeholders::_2));
  CMD2_ANY         ("network.local_address",       std::bind(&core::Manager::local_address, control->core()));
  CMD2_ANY_STRING_V("network.local_address.set",   std::bind(&core::Manager::set_local_address, control->core(), std::placeholders::_2));
  CMD2_ANY         ("network.proxy_address",       std::bind(&core::Manager::proxy_address, control->core()));
  CMD2_ANY_STRING_V("network.proxy_address.set",   std::bind(&core::Manager::set_proxy_address, control->core(), std::placeholders::_2));

  CMD2_ANY         ("network.max_open_files",       std::bind(&torrent::FileManager::max_open_files, fileManager));
  CMD2_ANY_VALUE_V ("network.max_open_files.set",   std::bind(&torrent::FileManager::set_max_open_files, fileManager, std::placeholders::_2));
  CMD2_ANY         ("network.open_sockets",         std::bind(&torrent::ConnectionManager::size, cm));
  CMD2_ANY         ("network.max_open_sockets",     std::bind(&torrent::ConnectionManager::max_size, cm));
  CMD2_ANY_VALUE_V ("network.max_open_sockets.set", std::bind(&torrent::ConnectionManager::set_max_size, cm, std::placeholders::_2));

  CMD2_ANY_STRING  ("network.scgi.open_port",   std::bind(&apply_scgi, std::placeholders::_2, 1));
  CMD2_ANY_STRING  ("network.scgi.open_local",  std::bind(&apply_scgi, std::placeholders::_2, 2));
  CMD2_VAR_BOOL    ("network.scgi.dont_route",  false);

  CMD2_ANY_STRING  ("network.xmlrpc.dialect.set",    std::bind(&apply_xmlrpc_dialect, std::placeholders::_2));
  CMD2_ANY         ("network.xmlrpc.size_limit",     std::bind(&rpc::XmlRpc::size_limit));
  CMD2_ANY_VALUE_V ("network.xmlrpc.size_limit.set", std::bind(&rpc::XmlRpc::set_size_limit, std::placeholders::_2));

  CMD2_ANY         ("system.hash.read_ahead",        std::bind(&torrent::hash_read_ahead));
  CMD2_ANY_VALUE_V ("system.hash.read_ahead.set",    std::bind(&apply_hash_read_ahead, std::placeholders::_2));
  CMD2_ANY         ("system.hash.interval",          std::bind(&torrent::hash_interval));
  CMD2_ANY_VALUE_V ("system.hash.interval.set",      std::bind(&apply_hash_interval, std::placeholders::_2));
  CMD2_ANY         ("system.hash.max_tries",         std::bind(&torrent::hash_max_tries));
  CMD2_ANY_VALUE_V ("system.hash.max_tries.set",     std::bind(&torrent::set_hash_max_tries, std::placeholders::_2));

  CMD2_ANY_VALUE   ("trackers.enable",  std::bind(&apply_enable_trackers, int64_t(1)));
  CMD2_ANY_VALUE   ("trackers.disable", std::bind(&apply_enable_trackers, int64_t(0)));
  CMD2_VAR_VALUE   ("trackers.numwant", -1);
  CMD2_VAR_BOOL    ("trackers.use_udp", true);

  CMD2_ANY_STRING  ("ip_tables.insert_table", std::bind(&apply_ip_tables_insert_table, std::placeholders::_2));
  CMD2_ANY_LIST    ("ip_tables.get",          std::bind(&apply_ip_tables_get, std::placeholders::_2));
  CMD2_ANY_LIST    ("ip_tables.add_address",  std::bind(&apply_ip_tables_add_address, std::placeholders::_2));

  CMD2_ANY         ("ipv4_filter.size_data",   std::bind(&apply_ipv4_filter_size_data));
  CMD2_ANY_STRING  ("ipv4_filter.get",         std::bind(&apply_ipv4_filter_get, std::placeholders::_2));
  CMD2_ANY_LIST    ("ipv4_filter.add_address", std::bind(&apply_ipv4_filter_add_address, std::placeholders::_2));
  CMD2_ANY_LIST    ("ipv4_filter.load",        std::bind(&apply_ipv4_filter_load, std::placeholders::_2));

//   CMD2_ANY_V       ("dht.enable",     std::bind(&core::DhtManager::set_start, control->dht_manager()));
//   CMD2_ANY_V       ("dht.disable",    std::bind(&core::DhtManager::set_stop, control->dht_manager()));
  CMD2_ANY_STRING_V("dht.mode.set",          std::bind(&core::DhtManager::set_mode, control->dht_manager(), std::placeholders::_2));
  CMD2_VAR_VALUE   ("dht.port",              int64_t(6881));
  CMD2_ANY_STRING  ("dht.add_node",          std::bind(&apply_dht_add_node, std::placeholders::_2));
  CMD2_ANY         ("dht.statistics",        std::bind(&core::DhtManager::dht_statistics, control->dht_manager()));
  CMD2_ANY         ("dht.throttle.name",     std::bind(&core::DhtManager::throttle_name, control->dht_manager()));
  CMD2_ANY_STRING_V("dht.throttle.name.set", std::bind(&core::DhtManager::set_throttle_name, control->dht_manager(), std::placeholders::_2));
}
