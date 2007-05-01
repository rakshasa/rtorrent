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
#include <torrent/torrent.h>
#include <torrent/connection_manager.h>

#include "core/download_list.h"
#include "core/manager.h"
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

void
initialize_command_network() {
  utils::VariableMap* variables = control->variable();
//   core::DownloadList* downloadList = control->core()->download_list();

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

  ADD_COMMAND_SLOT("encryption",       call_list, rak::ptr_fn(&apply_encryption));

  ADD_COMMAND_SLOT("tos",              call_string, rak::ptr_fn(&apply_tos));

  ADD_COMMAND_STRING_TRI("bind",          rak::make_mem_fun(control->core(), &core::Manager::set_bind_address), rak::make_mem_fun(control->core(), &core::Manager::bind_address));
  ADD_COMMAND_STRING_TRI("ip",            rak::make_mem_fun(control->core(), &core::Manager::set_local_address), rak::make_mem_fun(control->core(), &core::Manager::local_address));
  ADD_COMMAND_STRING_TRI("proxy_address", rak::make_mem_fun(control->core(), &core::Manager::set_proxy_address), rak::make_mem_fun(control->core(), &core::Manager::proxy_address));
}
