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

#include "core/dht_manager.h"
#include "core/download.h"
#include "core/manager.h"
#include "rpc/scgi.h"
#include "ui/root.h"
#include "rpc/command_slot.h"
#include "rpc/command_variable.h"
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

void apply_hash_read_ahead(int arg)              { torrent::set_hash_read_ahead(arg << 20); }
void apply_hash_interval(int arg)                { torrent::set_hash_interval(arg * 1000); }
void apply_encoding_list(const std::string& arg) { torrent::encoding_list()->push_back(arg); }

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

void
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
}

void
apply_enable_trackers(int64_t arg) {
  for (core::Manager::DListItr itr = control->core()->download_list()->begin(), last = control->core()->download_list()->end(); itr != last; ++itr) {
    std::for_each((*itr)->tracker_list()->begin(), (*itr)->tracker_list()->end(),
                  arg ? std::mem_fun(&torrent::Tracker::enable) : std::mem_fun(&torrent::Tracker::disable));

    if (arg && !rpc::call_command_value("get_use_udp_trackers"))
      (*itr)->enable_udp_trackers(false);
  }    
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

void
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
      scgi->open_port(saPtr, saPtr->length(), rpc::call_command_value("get_scgi_dont_route"));

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
}

void
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
}

void
initialize_command_network() {
  torrent::ConnectionManager* cm = torrent::connection_manager();
  core::CurlStack* httpStack = control->core()->http_stack();

  ADD_VARIABLE_BOOL("use_udp_trackers", true);

  // Isn't port_open used?
  ADD_VARIABLE_BOOL("port_open", true);
  ADD_VARIABLE_BOOL("port_random", true);
  ADD_VARIABLE_STRING("port_range", "6881-6999");

  ADD_VARIABLE_STRING("connection_leech", "leech");
  ADD_VARIABLE_STRING("connection_seed", "seed");

  ADD_VARIABLE_VALUE("min_peers", 40);
  ADD_VARIABLE_VALUE("max_peers", 100);
  ADD_VARIABLE_VALUE("min_peers_seed", -1);
  ADD_VARIABLE_VALUE("max_peers_seed", -1);

  ADD_VARIABLE_VALUE("max_uploads", 15);

  ADD_VARIABLE_VALUE("max_uploads_div",      1);
  ADD_VARIABLE_VALUE("max_uploads_global",   0);
  ADD_VARIABLE_VALUE("max_downloads_div",    1);
  ADD_VARIABLE_VALUE("max_downloads_global", 0);

//   ADD_COMMAND_VALUE_TRI("max_uploads_global",   rak::make_mem_fun(control->ui(), &ui::Root::set_max_uploads_global), rak::make_mem_fun(control->ui(), &ui::Root::max_uploads_global));
//   ADD_COMMAND_VALUE_TRI("max_downloads_global", rak::make_mem_fun(control->ui(), &ui::Root::set_max_downloads_global), rak::make_mem_fun(control->ui(), &ui::Root::max_downloads_global));

  ADD_COMMAND_VALUE_TRI_KB("download_rate",     rak::make_mem_fun(control->ui(), &ui::Root::set_down_throttle_i64), rak::make_mem_fun(torrent::down_throttle_global(), &torrent::Throttle::max_rate));
  ADD_COMMAND_VALUE_TRI_KB("upload_rate",       rak::make_mem_fun(control->ui(), &ui::Root::set_up_throttle_i64), rak::make_mem_fun(torrent::up_throttle_global(), &torrent::Throttle::max_rate));

  ADD_COMMAND_VOID("get_up_rate",               rak::make_mem_fun(torrent::up_rate(), &torrent::Rate::rate));
  ADD_COMMAND_VOID("get_up_total",              rak::make_mem_fun(torrent::up_rate(), &torrent::Rate::total));
  ADD_COMMAND_VOID("get_down_rate",             rak::make_mem_fun(torrent::down_rate(), &torrent::Rate::rate));
  ADD_COMMAND_VOID("get_down_total",            rak::make_mem_fun(torrent::down_rate(), &torrent::Rate::total));

  ADD_VARIABLE_VALUE("tracker_numwant", -1);

  CMD2_ANY_LIST("throttle_up",         std::tr1::bind(&apply_throttle, std::tr1::placeholders::_2, true));
  CMD2_ANY_LIST("throttle_down",       std::tr1::bind(&apply_throttle, std::tr1::placeholders::_2, false));
  CMD2_ANY_LIST("throttle_ip",         std::tr1::bind(&apply_address_throttle, std::tr1::placeholders::_2));

  CMD2_ANY_STRING("get_throttle_up_max",    std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_up | throttle_info_max));
  CMD2_ANY_STRING("get_throttle_up_rate",   std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_up | throttle_info_rate));
  CMD2_ANY_STRING("get_throttle_down_max",  std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_down | throttle_info_max));
  CMD2_ANY_STRING("get_throttle_down_rate", std::tr1::bind(&retrieve_throttle_info, std::tr1::placeholders::_2, throttle_info_down | throttle_info_rate));

  CMD2_ANY_STRING("tos",                  std::tr1::bind(&apply_tos, std::tr1::placeholders::_2));

  CMD2_ANY_LIST("encryption",          std::tr1::bind(&apply_encryption, std::tr1::placeholders::_2));

  ADD_COMMAND_STRING_TRI("bind",          rak::make_mem_fun(control->core(), &core::Manager::set_bind_address), rak::make_mem_fun(control->core(), &core::Manager::bind_address));
  ADD_COMMAND_STRING_TRI("ip",            rak::make_mem_fun(control->core(), &core::Manager::set_local_address), rak::make_mem_fun(control->core(), &core::Manager::local_address));
  ADD_COMMAND_STRING_TRI("proxy_address", rak::make_mem_fun(control->core(), &core::Manager::set_proxy_address), rak::make_mem_fun(control->core(), &core::Manager::proxy_address));
  ADD_COMMAND_STRING_TRI("http_proxy",    rak::make_mem_fun(httpStack, &core::CurlStack::set_http_proxy), rak::make_mem_fun(httpStack, &core::CurlStack::http_proxy));
  ADD_COMMAND_STRING_TRI("http_capath",   rak::make_mem_fun(httpStack, &core::CurlStack::set_http_capath), rak::make_mem_fun(httpStack, &core::CurlStack::http_capath));
  ADD_COMMAND_STRING_TRI("http_cacert",   rak::make_mem_fun(httpStack, &core::CurlStack::set_http_cacert), rak::make_mem_fun(httpStack, &core::CurlStack::http_cacert));

  ADD_COMMAND_VALUE_TRI("send_buffer_size",    rak::make_mem_fun(cm, &torrent::ConnectionManager::set_send_buffer_size), rak::make_mem_fun(cm, &torrent::ConnectionManager::send_buffer_size));
  ADD_COMMAND_VALUE_TRI("receive_buffer_size", rak::make_mem_fun(cm, &torrent::ConnectionManager::set_receive_buffer_size), rak::make_mem_fun(cm, &torrent::ConnectionManager::receive_buffer_size));

  ADD_COMMAND_VALUE_TRI("hash_max_tries",       std::ptr_fun(&torrent::set_hash_max_tries), rak::ptr_fun(&torrent::hash_max_tries));
  ADD_COMMAND_VALUE_TRI("max_open_files",       std::ptr_fun(&torrent::set_max_open_files), rak::ptr_fun(&torrent::max_open_files));
  ADD_COMMAND_VALUE_TRI("max_open_sockets",     rak::make_mem_fun(cm, &torrent::ConnectionManager::set_max_size), rak::make_mem_fun(cm, &torrent::ConnectionManager::max_size));
  ADD_COMMAND_VALUE_TRI("max_open_http",        rak::make_mem_fun(httpStack, &core::CurlStack::set_max_active), rak::make_mem_fun(httpStack, &core::CurlStack::max_active));

  ADD_COMMAND_STRING_UN("scgi_port",            rak::bind2nd(std::ptr_fun(&apply_scgi), 1));
  ADD_COMMAND_STRING_UN("scgi_local",           rak::bind2nd(std::ptr_fun(&apply_scgi), 2));
  ADD_VARIABLE_BOOL    ("scgi_dont_route", false);
  ADD_COMMAND_STRING_UN("xmlrpc_dialect",       std::ptr_fun(&apply_xmlrpc_dialect));
  ADD_COMMAND_VALUE_TRI("xmlrpc_size_limit",    std::ptr_fun(&rpc::XmlRpc::set_size_limit), rak::ptr_fun(&rpc::XmlRpc::size_limit));

  ADD_COMMAND_VALUE_TRI("hash_read_ahead",      std::ptr_fun(&apply_hash_read_ahead), rak::ptr_fun(torrent::hash_read_ahead));
  ADD_COMMAND_VALUE_TRI("hash_interval",        std::ptr_fun(&apply_hash_interval), rak::ptr_fun(torrent::hash_interval));

  ADD_COMMAND_VALUE_UN("enable_trackers",       std::ptr_fun(&apply_enable_trackers));
  ADD_COMMAND_STRING_UN("encoding_list",        std::ptr_fun(&apply_encoding_list));

  ADD_VARIABLE_VALUE("dht_port", 6881);
  ADD_COMMAND_STRING_UN("dht",                  rak::make_mem_fun(control->dht_manager(), &core::DhtManager::set_start));
  ADD_COMMAND_STRING_UN("dht_add_node",         std::ptr_fun(&apply_dht_add_node));
  ADD_COMMAND_VOID("dht_statistics",            rak::make_mem_fun(control->dht_manager(), &core::DhtManager::dht_statistics));
  ADD_COMMAND_STRING_TRI("dht_throttle",        rak::make_mem_fun(control->dht_manager(), &core::DhtManager::set_throttle_name), rak::make_mem_fun(control->dht_manager(), &core::DhtManager::throttle_name));

  ADD_VARIABLE_BOOL("peer_exchange", true);

  rpc::commands.call("method.insert", rpc::create_object_list("log.handshake", "bool|const", int64_t()));
  rpc::commands.call("method.insert", rpc::create_object_list("log.tracker", "string|const", ""));
}
