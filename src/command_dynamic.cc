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

#include <algorithm>

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
system_method_insert(__UNUSED rpc::target_type target, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  // Allow arbritary number of args, just use what ever necessary in
  // order to create a function blob out of them.

  if (args.empty())
    return torrent::Object();

  const std::string& rawKey = args.front().as_string();

  if (rawKey.empty() || rpc::commands.has(rawKey))
    throw torrent::input_error("Invalid key.");

  std::string command;

  for (torrent::Object::list_const_iterator itr = ++args.begin(), last = args.end(); itr != last; itr++) {
    if (!command.empty())
      command += " ;";

    command += itr->as_string();
  }

  char* key = new char[rawKey.size() + 1];
  std::memcpy(key, rawKey.c_str(), rawKey.size() + 1);

  rpc::commands.insert_type(key, new rpc::CommandFunction(command), &rpc::CommandFunction::call,
                            rpc::CommandMap::flag_delete_key | rpc::CommandMap::flag_modifiable | rpc::CommandMap::flag_public_xmlrpc, NULL, NULL);

  return torrent::Object();
}

torrent::Object
system_method_erase(__UNUSED rpc::target_type target, const torrent::Object& rawArgs) {
  rpc::CommandMap::iterator itr = rpc::commands.find(rawArgs.as_string().c_str());

  if (itr == rpc::commands.end())
    return torrent::Object();

  if (!(itr->second.m_flags & rpc::CommandMap::flag_modifiable))
    throw torrent::input_error("Command not modifiable.");

  rpc::commands.erase(itr);

  return torrent::Object();
}

void
initialize_command_dynamic() {
  CMD_G("system.method.insert", rak::ptr_fn(&system_method_insert));
  CMD_G_STRING("system.method.erase", rak::ptr_fn(&system_method_erase));
}
