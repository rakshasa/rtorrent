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
#include <cstdio>
#include <rak/address_info.h>
#include <rak/path.h>
#include <torrent/connection_manager.h>
#include <torrent/dht_manager.h>
#include <torrent/throttle.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>
#include <torrent/torrent.h>
#include <torrent/rate.h>
#include <torrent/data/file_manager.h>

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
apply_throttle(const torrent::Object::list_type& args, bool up) {
  torrent::Object::list_const_iterator argItr = args.begin();

  const std::string& name = argItr->as_string();
  if (name.empty() || name == "NULL")
    throw torrent::input_error("Invalid throttle name.");

  if ((++argItr)->as_string().empty())
    return torrent::Object();

  int64_t rate;
  rpc::parse_whole_value_nothrow(argItr->as_string().c_str(), &rate);

  if (rate < 0)
    throw torrent::input_error("Throttle rate must be non-negative.");

  core::ThrottleMap::iterator itr = control->core()->throttles().find(name);
  if (itr == control->core()->throttles().end())
    itr = control->core()->throttles().insert(std::make_pair(name, torrent::ThrottlePair(NULL, NULL))).first;

  torrent::Throttle*& throttle = up ? itr->second.first : itr->second.second;
  if (rate != 0 && throttle == NULL)
    throttle = (up ? torrent::up_throttle_global() : torrent::down_throttle_global())->create_slave();

  if (throttle != NULL)
    throttle->set_max_rate(rate * 1024);

  return torrent::Object();
}

static const int throttle_info_up   = (1 << 0);
static const int throttle_info_down = (1 << 1);
static const int throttle_info_max  = (1 << 2);
static const int throttle_info_rate = (1 << 3);

torrent::Object
retrieve_throttle_info(const torrent::Object::string_type& name, int flags) {
  core::ThrottleMap::iterator itr = control->core()->throttles().find(name);
  torrent::ThrottlePair throttles = itr == control->core()->throttles().end() ? torrent::ThrottlePair(NULL, NULL) : itr->second;
  torrent::Throttle* throttle = flags & throttle_info_down ? throttles.second : throttles.first;
  torrent::Throttle* global = flags & throttle_info_down ? torrent::down_throttle_global() : torrent::up_throttle_global();

  if (throttle == NULL && name.empty())
    throttle = global;

  if (throttle == NULL)
    return flags & throttle_info_rate ? (int64_t)0 : (int64_t)-1;
  else if (!throttle->is_throttled() || !global->is_throttled())
    return (int64_t)0;
  else if (flags & throttle_info_rate)
    return (int64_t)throttle->rate()->rate();
  else
    return (int64_t)throttle->max_rate();
}

std::pair<uint32_t, uint32_t>
parse_address_range(const torrent::Object::list_type& args, torrent::Object::list_type::const_iterator itr) {
  unsigned int prefixWidth, ret;
  char dummy;
  char host[1024];
  rak::address_info* ai;

  ret = std::sscanf(itr->as_string().c_str(), "%1023[^/]/%d%c", host, &prefixWidth, &dummy);
  if (ret < 1 || rak::address_info::get_address_info(host, PF_INET, SOCK_STREAM, &ai) != 0)
    throw torrent::input_error("Could not resolve host.");

  uint32_t begin, end;
  rak::socket_address sa;
  sa.copy(*ai->address(), ai->length());
  begin = end = sa.sa_inet()->address_h();
  rak::address_info::free_address_info(ai);

  if (ret == 2) {
    if (++itr != args.end())
      throw torrent::input_error("Cannot specify both network and range end.");

    uint32_t netmask = std::numeric_limits<uint32_t>::max() << (32 - prefixWidth);
    if (prefixWidth >= 32 || sa.sa_inet()->address_h() & ~netmask)
      throw torrent::input_error("Invalid address/prefix.");

    end = sa.sa_inet()->address_h() | ~netmask;

  } else if (++itr != args.end()) {
    if (rak::address_info::get_address_info(itr->as_string().c_str(), PF_INET, SOCK_STREAM, &ai) != 0)
      throw torrent::input_error("Could not resolve host.");

    sa.copy(*ai->address(), ai->length());
    rak::address_info::free_address_info(ai);
    end = sa.sa_inet()->address_h();
  }

  // convert to [begin, end) making sure the end doesn't overflow
  // (this precludes 255.255.255.255 from ever matching, but that's not a real IP anyway)
  return std::make_pair<uint32_t, uint32_t>(begin, std::max(end, end + 1));
}

torrent::Object
apply_address_throttle(const torrent::Object::list_type& args) {
  if (args.size() < 2 || args.size() > 3)
    throw torrent::input_error("Incorrect number of arguments.");

  std::pair<uint32_t, uint32_t> range = parse_address_range(args, ++args.begin());
  core::ThrottleMap::iterator throttleItr = control->core()->throttles().find(args.begin()->as_string().c_str());
  if (throttleItr == control->core()->throttles().end())
    throw torrent::input_error("Throttle not found.");

  control->core()->set_address_throttle(range.first, range.second, throttleItr->second);
  return torrent::Object();
}

torrent::Object
apply_encryption(const torrent::Object::list_type& args) {
  uint32_t options_mask = torrent::ConnectionManager::encryption_none;

  for (torrent::Object::list_const_iterator itr = args.begin(), last = args.end(); itr != last; itr++) {
    const std::string& opt = itr->as_string();

    if (opt == "none")
      options_mask = torrent::ConnectionManager::encryption_none;
    else if (opt == "allow_incoming")
      options_mask |= torrent::ConnectionManager::encryption_allow_incoming;
    else if (opt == "try_outgoing")
      options_mask |= torrent::ConnectionManager::encryption_try_outgoing;
    else if (opt == "require")
      options_mask |= torrent::ConnectionManager::encryption_require;
    else if (opt == "require_RC4" || opt == "require_rc4")
      options_mask |= torrent::ConnectionManager::encryption_require_RC4;
    else if (opt == "enable_retry")
      options_mask |= torrent::ConnectionManager::encryption_enable_retry;
    else if (opt == "prefer_plaintext")
      options_mask |= torrent::ConnectionManager::encryption_prefer_plaintext;
    else
      throw torrent::input_error("Invalid encryption option.");
  }

  torrent::connection_manager()->set_encryption_options(options_mask);

  return torrent::Object();
}

torrent::Object
apply_tos(const torrent::Object::string_type& arg) {
  rpc::Command::value_type value;
  torrent::ConnectionManager* cm = torrent::connection_manager();

  if (arg == "default")
    value = torrent::ConnectionManager::iptos_default;
  else if (arg == "lowdelay")
    value = torrent::ConnectionManager::iptos_lowdelay;
  else if (arg == "throughput")
    value = torrent::ConnectionManager::iptos_throughput;
  else if (arg == "reliability")
    value = torrent::ConnectionManager::iptos_reliability;
  else if (arg == "mincost")
    value = torrent::ConnectionManager::iptos_mincost;
  else if (!rpc::parse_whole_value_nothrow(arg.c_str(), &value, 16, 1))
    throw torrent::input_error("Invalid TOS identifier.");

  cm->set_priority(value);

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

void
initialize_xmlrpc() {
  rpc::xmlrpc.initialize();
  rpc::xmlrpc.set_slot_find_download(rak::mem_fn(control->core()->download_list(), &core::DownloadList::find_hex_ptr));
  rpc::xmlrpc.set_slot_find_file(rak::ptr_fn(&xmlrpc_find_file));
  rpc::xmlrpc.set_slot_find_tracker(rak::ptr_fn(&xmlrpc_find_tracker));

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

void
initialize_command_network() {
  torrent::ConnectionManager* cm = torrent::connection_manager();
  torrent::FileManager* fileManager = torrent::file_manager();
  core::CurlStack* httpStack = control->core()->http_stack();

  CMD2_VAR_BOOL    ("protocol.pex",  true);
  CMD2_VAR_BOOL    ("log.handshake", false);
  CMD2_VAR_STRING  ("log.tracker",   "");

  CMD2_ANY_STRING  ("encoding_list",    std::tr1::bind(&apply_encoding_list, std::tr1::placeholders::_2));

  // Isn't port_open used?
  CMD2_VAR_BOOL  ("network.port_open", true);
  CMD2_VAR_BOOL  ("network.port_random", true);
  CMD2_VAR_STRING("network.port_range", "6881-6999");

  CMD2_VAR_STRING("connection_leech", "leech");
  CMD2_VAR_STRING("connection_seed", "seed");

  CMD2_VAR_VALUE   ("throttle.min_peers.normal", 40);
  CMD2_VAR_VALUE   ("throttle.max_peers.normal", 100);
  CMD2_VAR_VALUE   ("throttle.min_peers.seed",   -1);
  CMD2_VAR_VALUE   ("throttle.max_peers.seed",   -1);

  CMD2_VAR_VALUE   ("throttle.max_uploads", 15);

  CMD2_VAR_VALUE   ("throttle.max_uploads.div",      1);
  CMD2_VAR_VALUE   ("throttle.max_uploads.global",   0);
  CMD2_VAR_VALUE   ("throttle.max_downloads.div",    1);
  CMD2_VAR_VALUE   ("throttle.max_downloads.global", 0);

  // TODO: Move the logic into some libtorrent function.
  CMD2_ANY         ("throttle.global_up.rate",              std::tr1::bind(&torrent::Rate::rate, torrent::up_rate()));
  CMD2_ANY         ("throttle.global_up.total",             std::tr1::bind(&torrent::Rate::total, torrent::up_rate()));
  CMD2_ANY         ("throttle.global_up.max_rate",          std::tr1::bind(&torrent::Throttle::max_rate, torrent::up_throttle_global()));
  CMD2_ANY_VALUE_V ("throttle.global_up.max_rate.set",      std::tr1::bind(&ui::Root::set_up_throttle_i64, control->ui(), std::tr1::placeholders::_2));
  CMD2_ANY_VALUE_KB("throttle.global_up.max_rate.set_kb",   std::tr1::bind(&ui::Root::set_up_throttle_i64, control->ui(), std::tr1::placeholders::_2));
  CMD2_ANY         ("throttle.global_down.rate",            std::tr1::bind(&torrent::Rate::rate, torrent::down_rate()));
  CMD2_ANY         ("throttle.global_down.total",           std::tr1::bind(&torrent::Rate::total, torrent::down_rate()));
  CMD2_ANY         ("throttle.global_down.max_rate",        std::tr1::bind(&torrent::Throttle::max_rate, torrent::down_throttle_global()));
  CMD2_ANY_VALUE_V ("throttle.global_down.max_rate.set",    std::tr1::bind(&ui::Root::set_down_throttle_i64, control->ui(), std::tr1::placeholders::_2));
  CMD2_ANY_VALUE_KB("throttle.global_down.max_rate.set_kb", std::tr1::bind(&ui::Root::set_down_throttle_i64, control->ui(), std::tr1::placeholders::_2));

  CMD2_ANY_LIST("throttle_up",         std::tr1::bind(&apply_throttle, std::tr1::placeholders::_2, true));
  CMD2_ANY_LIST("throttle_down",       std::tr1::bind(&apply_throttle, std::tr1::placeholders::_2, false));
  CMD2_ANY_LIST("throttle_ip",         std::tr1::bind(&apply_address_throttle, std::tr1::placeholders::_2));

  CMD2_ANY_STRING("get_throttle_up_max",    std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_up | throttle_info_max));
  CMD2_ANY_STRING("get_throttle_up_rate",   std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_up | throttle_info_rate));
  CMD2_ANY_STRING("get_throttle_down_max",  std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_down | throttle_info_max));
  CMD2_ANY_STRING("get_throttle_down_rate", std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_down | throttle_info_rate));

  CMD2_ANY_LIST("encryption",          std::tr1::bind(&apply_encryption, std::tr1::placeholders::_2));

  CMD2_ANY         ("network.http.capath",            std::tr1::bind(&core::CurlStack::http_capath, httpStack));
  CMD2_ANY_STRING_V("network.http.capath.set",        std::tr1::bind(&core::CurlStack::set_http_capath, httpStack, std::tr1::placeholders::_2));
  CMD2_ANY         ("network.http.cacert",            std::tr1::bind(&core::CurlStack::http_cacert, httpStack));
  CMD2_ANY_STRING_V("network.http.cacert.set",        std::tr1::bind(&core::CurlStack::set_http_cacert, httpStack, std::tr1::placeholders::_2));
  CMD2_ANY         ("network.http.proxy_address",     std::tr1::bind(&core::CurlStack::http_proxy, httpStack));
  CMD2_ANY_STRING_V("network.http.proxy_address.set", std::tr1::bind(&core::CurlStack::set_http_proxy, httpStack, std::tr1::placeholders::_2));
  CMD2_ANY         ("network.http.max_open",          std::tr1::bind(&core::CurlStack::max_active, httpStack));
  CMD2_ANY_VALUE_V ("network.http.max_open.set",      std::tr1::bind(&core::CurlStack::set_max_active, httpStack, std::tr1::placeholders::_2));

  CMD2_ANY         ("network.send_buffer.size",        std::tr1::bind(&torrent::ConnectionManager::send_buffer_size, cm));
  CMD2_ANY_VALUE_V ("network.send_buffer.size.set",    std::tr1::bind(&torrent::ConnectionManager::set_send_buffer_size, cm, std::tr1::placeholders::_2));
  CMD2_ANY         ("network.receive_buffer.size",     std::tr1::bind(&torrent::ConnectionManager::receive_buffer_size, cm));
  CMD2_ANY_VALUE_V ("network.receive_buffer.size.set", std::tr1::bind(&torrent::ConnectionManager::set_receive_buffer_size, cm, std::tr1::placeholders::_2));
  CMD2_ANY_STRING  ("network.tos.set",                 std::tr1::bind(&apply_tos, std::tr1::placeholders::_2));

  CMD2_ANY         ("network.bind_address",        std::tr1::bind(&core::Manager::bind_address, control->core()));
  CMD2_ANY_STRING_V("network.bind_address.set",    std::tr1::bind(&core::Manager::set_bind_address, control->core(), std::tr1::placeholders::_2));
  CMD2_ANY         ("network.local_address",       std::tr1::bind(&core::Manager::local_address, control->core()));
  CMD2_ANY_STRING_V("network.local_address.set",   std::tr1::bind(&core::Manager::set_local_address, control->core(), std::tr1::placeholders::_2));
  CMD2_ANY         ("network.proxy_address",       std::tr1::bind(&core::Manager::proxy_address, control->core()));
  CMD2_ANY_STRING_V("network.proxy_address.set",   std::tr1::bind(&core::Manager::set_proxy_address, control->core(), std::tr1::placeholders::_2));

  CMD2_ANY         ("network.max_open_files",       std::tr1::bind(&torrent::FileManager::max_open_files, fileManager));
  CMD2_ANY_VALUE_V ("network.max_open_files.set",   std::tr1::bind(&torrent::FileManager::set_max_open_files, fileManager, std::tr1::placeholders::_2));
  CMD2_ANY         ("network.max_open_sockets",     std::tr1::bind(&torrent::ConnectionManager::max_size, cm));
  CMD2_ANY_VALUE_V ("network.max_open_sockets.set", std::tr1::bind(&torrent::ConnectionManager::set_max_size, cm, std::tr1::placeholders::_2));

  CMD2_ANY_STRING  ("network.scgi.open_port",   std::tr1::bind(&apply_scgi, std::tr1::placeholders::_2, 1));
  CMD2_ANY_STRING  ("network.scgi.open_local",  std::tr1::bind(&apply_scgi, std::tr1::placeholders::_2, 2));
  CMD2_VAR_BOOL    ("network.scgi.dont_route",  false);

  CMD2_ANY_STRING  ("network.xmlrpc.dialect.set",    std::tr1::bind(&apply_xmlrpc_dialect, std::tr1::placeholders::_2));
  CMD2_ANY         ("network.xmlrpc.size_limit",     std::tr1::bind(&rpc::XmlRpc::size_limit));
  CMD2_ANY_VALUE_V ("network.xmlrpc.size_limit.set", std::tr1::bind(&rpc::XmlRpc::set_size_limit, std::tr1::placeholders::_2));

  CMD2_ANY         ("system.hash.read_ahead",        std::tr1::bind(&torrent::hash_read_ahead));
  CMD2_ANY_VALUE_V ("system.hash.read_ahead.set",    std::tr1::bind(&apply_hash_read_ahead, std::tr1::placeholders::_2));
  CMD2_ANY         ("system.hash.interval",          std::tr1::bind(&torrent::hash_interval));
  CMD2_ANY_VALUE_V ("system.hash.interval.set",      std::tr1::bind(&apply_hash_interval, std::tr1::placeholders::_2));
  CMD2_ANY         ("system.hash.max_tries",         std::tr1::bind(&torrent::hash_max_tries));
  CMD2_ANY_VALUE_V ("system.hash.max_tries.set",     std::tr1::bind(&torrent::set_hash_max_tries, std::tr1::placeholders::_2));

  CMD2_ANY_VALUE   ("trackers.enable",  std::tr1::bind(&apply_enable_trackers, int64_t(1)));
  CMD2_ANY_VALUE   ("trackers.disable", std::tr1::bind(&apply_enable_trackers, int64_t(0)));
  CMD2_VAR_VALUE   ("trackers.numwant", -1);
  CMD2_VAR_BOOL    ("trackers.use_udp", true);

//   CMD2_ANY_V       ("dht.enable",     std::tr1::bind(&core::DhtManager::set_start, control->dht_manager()));
//   CMD2_ANY_V       ("dht.disable",    std::tr1::bind(&core::DhtManager::set_stop, control->dht_manager()));
  CMD2_ANY_STRING_V("dht.mode.set",          std::tr1::bind(&core::DhtManager::set_mode, control->dht_manager(), std::tr1::placeholders::_2));
  CMD2_VAR_VALUE   ("dht.port",              int64_t(6881));
  CMD2_ANY_STRING  ("dht.add_node",          std::tr1::bind(&apply_dht_add_node, std::tr1::placeholders::_2));
  CMD2_ANY         ("dht.statistics",        std::tr1::bind(&core::DhtManager::dht_statistics, control->dht_manager()));
  CMD2_ANY         ("dht.throttle.name",     std::tr1::bind(&core::DhtManager::throttle_name, control->dht_manager()));
  CMD2_ANY_STRING_V("dht.throttle.name.set", std::tr1::bind(&core::DhtManager::set_throttle_name, control->dht_manager(), std::tr1::placeholders::_2));
}
