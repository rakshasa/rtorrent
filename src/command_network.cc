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

#include <functional>
#include <rak/file_stat.h>
#include <rak/path.h>
#include <torrent/connection_manager.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>
#include <torrent/torrent.h>

#include "core/download.h"
#include "core/download_list.h"
#include "core/download_store.h"
#include "core/manager.h"
#include "rpc/fast_cgi.h"
#include "rpc/scgi.h"
#include "rpc/xmlrpc.h"
#include "ui/root.h"
#include "utils/command_slot.h"
#include "utils/command_variable.h"
#include "utils/variable_map.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
apply_encryption(const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  uint32_t options_mask = torrent::ConnectionManager::encryption_none;

  for (torrent::Object::list_type::const_iterator itr = args.begin(), last = args.end(); itr != last; itr++) {
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
      throw torrent::input_error("Invalid encryption option '" + opt + "'.");
  }

  torrent::connection_manager()->set_encryption_options(options_mask);

  return torrent::Object();
}

torrent::Object
apply_tos(const torrent::Object& rawArg) {
  utils::Variable::value_type value;
  torrent::ConnectionManager* cm = torrent::connection_manager();

  const std::string& arg = rawArg.as_string();

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

  else if (!utils::Variable::string_to_value_unit_nothrow(arg.c_str(), &value, 16, 1))
    throw torrent::input_error("Invalid TOS identifier.");

  cm->set_priority(value);

  return torrent::Object();
}

void apply_hash_read_ahead(int arg)              { torrent::set_hash_read_ahead(arg << 20); }
void apply_hash_interval(int arg)                { torrent::set_hash_interval(arg * 1000); }
void apply_encoding_list(const std::string& arg) { torrent::encoding_list()->push_back(arg); }

void
apply_enable_trackers(int64_t arg) {
  for (core::Manager::DListItr itr = control->core()->download_list()->begin(), last = control->core()->download_list()->end(); itr != last; ++itr) {

    torrent::TrackerList tl = (*itr)->download()->tracker_list();

    for (int i = 0, last = tl.size(); i < last; ++i)
      if (arg)
        tl.get(i).enable();
      else
        tl.get(i).disable();

    if (arg && !control->variable()->call_command_value("get_use_udp_trackers"))
      (*itr)->enable_udp_trackers(false);
  }    
}

void
initialize_xmlrpc() {
  control->set_xmlrpc(new rpc::XmlRpc);
  control->xmlrpc()->set_slot_call_command(rak::mem_fn(control->variable(), &utils::VariableMap::call_command));

  unsigned int count = 0;

  for (utils::VariableMap::const_iterator itr = control->variable()->begin(), last = control->variable()->end(); itr != last; itr++)
    if (itr->second.m_flags & utils::VariableMap::flag_public_xmlrpc) {
      control->xmlrpc()->insert_command(itr->first, itr->second.m_parm, itr->second.m_doc);

      count++;
    }

  char buffer[128];
  sprintf(buffer, "XMLRPC initialized with %u functions.", count);

  control->core()->push_log(buffer);
}

void
apply_fast_cgi(const std::string& arg) {
  if (control->fast_cgi() != NULL)
    throw torrent::input_error("FastCGI already enabled.");

  if (control->xmlrpc() == NULL)
    initialize_xmlrpc();

  control->set_fast_cgi(new rpc::FastCgi(arg));
  control->fast_cgi()->set_slot_process(rak::mem_fn(control->xmlrpc(), &rpc::XmlRpc::process));
}

void
apply_scgi(const std::string& arg) {
  if (control->fast_cgi() != NULL)
    throw torrent::input_error("FastCGI already enabled.");

  if (control->xmlrpc() == NULL)
    initialize_xmlrpc();

  // Fix this...
  control->set_scgi(new rpc::SCgi);

  try {
    int port;
    char dummy;

    if (std::sscanf(arg.c_str(), ":%i%c", &port, &dummy) == 1) {
      if (port <= 0 || port >= (1 << 16))
        throw torrent::input_error("Invalid port number.");

      control->scgi()->open_port(port);

    } else {
      control->scgi()->open_named(rak::path_expand(arg));
    }

  } catch (torrent::local_error& e) {
    throw torrent::input_error(e.what());
  }

  control->scgi()->set_slot_process(rak::mem_fn(control->xmlrpc(), &rpc::XmlRpc::process));
}

void
initialize_command_network() {
  utils::VariableMap* variables = control->variable();
//   core::DownloadList* downloadList = control->core()->download_list();
  torrent::ConnectionManager* cm = torrent::connection_manager();
  core::CurlStack* httpStack = control->core()->get_poll_manager()->get_http_stack();
  core::DownloadList*  dList = control->core()->download_list();
  core::DownloadStore* dStore = control->core()->download_store();

  ADD_VARIABLE_BOOL("use_udp_trackers", true);

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
  ADD_VARIABLE_VALUE("max_uploads_div", 1);

  ADD_VARIABLE_VALUE("max_downloads_div", 0);

  ADD_COMMAND_VALUE_TRI_KB("download_rate", rak::make_mem_fun(control->ui(), &ui::Root::set_down_throttle_i64), rak::ptr_fun(&torrent::down_throttle));
  ADD_COMMAND_VALUE_TRI_KB("upload_rate",   rak::make_mem_fun(control->ui(), &ui::Root::set_up_throttle_i64), rak::ptr_fun(&torrent::up_throttle));

  ADD_VARIABLE_VALUE("tracker_numwant", -1);

  ADD_COMMAND_LIST("encryption",          rak::ptr_fn(&apply_encryption));

  ADD_COMMAND_STRING("tos",               rak::ptr_fn(&apply_tos));

  ADD_COMMAND_STRING_TRI("bind",          rak::make_mem_fun(control->core(), &core::Manager::set_bind_address), rak::make_mem_fun(control->core(), &core::Manager::bind_address));
  ADD_COMMAND_STRING_TRI("ip",            rak::make_mem_fun(control->core(), &core::Manager::set_local_address), rak::make_mem_fun(control->core(), &core::Manager::local_address));
  ADD_COMMAND_STRING_TRI("proxy_address", rak::make_mem_fun(control->core(), &core::Manager::set_proxy_address), rak::make_mem_fun(control->core(), &core::Manager::proxy_address));
  ADD_COMMAND_STRING_TRI("http_proxy",    rak::make_mem_fun(httpStack, &core::CurlStack::set_http_proxy), rak::make_mem_fun(httpStack, &core::CurlStack::http_proxy));

  ADD_COMMAND_VALUE_TRI("send_buffer_size",    rak::make_mem_fun(cm, &torrent::ConnectionManager::set_send_buffer_size), rak::make_mem_fun(cm, &torrent::ConnectionManager::send_buffer_size));
  ADD_COMMAND_VALUE_TRI("receive_buffer_size", rak::make_mem_fun(cm, &torrent::ConnectionManager::set_receive_buffer_size), rak::make_mem_fun(cm, &torrent::ConnectionManager::receive_buffer_size));

  ADD_COMMAND_VALUE_TRI("max_uploads_global",   rak::make_mem_fun(control->ui(), &ui::Root::set_max_uploads_global), rak::make_mem_fun(control->ui(), &ui::Root::max_uploads_global));
  ADD_COMMAND_VALUE_TRI("max_downloads_global", rak::make_mem_fun(control->ui(), &ui::Root::set_max_downloads_global), rak::make_mem_fun(control->ui(), &ui::Root::max_downloads_global));

  ADD_COMMAND_VALUE_TRI("hash_max_tries",       std::ptr_fun(&torrent::set_hash_max_tries), rak::ptr_fun(&torrent::hash_max_tries));
  ADD_COMMAND_VALUE_TRI("max_open_files",       std::ptr_fun(&torrent::set_max_open_files), rak::ptr_fun(&torrent::max_open_files));
  ADD_COMMAND_VALUE_TRI("max_open_sockets",     rak::make_mem_fun(cm, &torrent::ConnectionManager::set_max_size), rak::make_mem_fun(cm, &torrent::ConnectionManager::max_size));
  ADD_COMMAND_VALUE_TRI("max_open_http",        rak::make_mem_fun(httpStack, &core::CurlStack::set_max_active), rak::make_mem_fun(httpStack, &core::CurlStack::max_active));

  ADD_COMMAND_STRING_UN("fast_cgi",             std::ptr_fun(&apply_fast_cgi));
  ADD_COMMAND_STRING_UN("scgi",                 std::ptr_fun(&apply_scgi));

  ADD_COMMAND_VALUE_TRI("hash_read_ahead",      std::ptr_fun(&apply_hash_read_ahead), rak::ptr_fun(torrent::hash_read_ahead));
  ADD_COMMAND_VALUE_TRI("hash_interval",        std::ptr_fun(&apply_hash_interval), rak::ptr_fun(torrent::hash_interval));

  ADD_COMMAND_VALUE_UN("enable_trackers",       std::ptr_fun(&apply_enable_trackers));
  ADD_COMMAND_STRING_UN("encoding_list",        std::ptr_fun(&apply_encoding_list));

  // Not really network stuff:
  ADD_VARIABLE_BOOL("handshake_log", false);
  ADD_VARIABLE_STRING("tracker_dump", "");

  ADD_VARIABLE_STRING("directory", "./");

  ADD_COMMAND_STRING_TRI("session",            rak::make_mem_fun(dStore, &core::DownloadStore::set_path), rak::make_mem_fun(dStore, &core::DownloadStore::path));
  ADD_COMMAND_VOID("session_save",             rak::make_mem_fun(dList, &core::DownloadList::session_save));

//   ADD_VARIABLE_VALUE_TRI_OCT("umask",               rak::mem_fn(control, &Control::set_umask), rak::mem_fn(control, &Control::umask));
  ADD_COMMAND_STRING_TRI("working_directory",  rak::make_mem_fun(control, &Control::set_working_directory), rak::make_mem_fun(control, &Control::working_directory));
}
