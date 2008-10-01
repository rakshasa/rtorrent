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

std::string
system_method_generate_command(torrent::Object::list_const_iterator first, torrent::Object::list_const_iterator last) {
  std::string command;

  while (first != last) {
    if (!command.empty())
      command += " ;";

    command += (first++)->as_string();
  }

  return command;
}

// system.method.insert <generic> {name, "simple|private|const", ...}
// system.method.insert <generic> {name, "list|private|const"}
//
// Add a new user-defined method called 'name' and any number of
// lines.
torrent::Object
system_method_insert(__UNUSED rpc::target_type target, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.empty() || ++args.begin() == args.end())
    throw torrent::input_error("Invalid argument count.");

  torrent::Object::list_const_iterator itrArgs = args.begin();
  const std::string& rawKey = (itrArgs++)->as_string();

  if (rawKey.empty() || rpc::commands.has(rawKey))
    throw torrent::input_error("Invalid key.");

  rpc::Command* command = NULL;
  rpc::Command::any_slot slot = NULL;
  int flags = rpc::CommandMap::flag_delete_key | rpc::CommandMap::flag_modifiable | rpc::CommandMap::flag_public_xmlrpc;

  const std::string& options = itrArgs->as_string();

  // Use a | separated list of options here, for non-modifable,
  // non-public methods, etc.
  if (options.find("list") != std::string::npos) {
    // Later, make it possible to add functions here.
    slot = &rpc::CommandFunctionList::call;
    command = new rpc::CommandFunctionList();

  } else if (options.find("simple") != std::string::npos) {
    slot = &rpc::CommandFunction::call;
    command = new rpc::CommandFunction(system_method_generate_command(++itrArgs, args.end()));
  }

  if (options.find("private") != std::string::npos)
    flags &= ~rpc::CommandMap::flag_public_xmlrpc;

  if (options.find("const") != std::string::npos)
    flags &= ~rpc::CommandMap::flag_modifiable;

  char* key = new char[rawKey.size() + 1];
  std::memcpy(key, rawKey.c_str(), rawKey.size() + 1);

  rpc::commands.insert_type(key, command, slot, flags, NULL, NULL);
  return torrent::Object();
}

// system.method.erase <> {name}
//
// Erase a modifiable method called 'name. Trying to remove methods
// that aren't modifiable, e.g. defined by rtorrent or set to
// read-only by the user, will result in an error.
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

torrent::Object
system_method_set(__UNUSED rpc::target_type target, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.empty())
    throw torrent::input_error("Invalid argument count.");

  rpc::CommandFunction* function;
  rpc::CommandMap::iterator itr = rpc::commands.find(args.front().as_string().c_str());

  if (itr == rpc::commands.end() || !(itr->second.m_flags & rpc::CommandMap::flag_modifiable) ||
      (function = dynamic_cast<rpc::CommandFunction*>(itr->second.m_variable)) == NULL)
    throw torrent::input_error("Command not modifiable or wrong type.");

  function->set_command(system_method_generate_command(++args.begin(), args.end()));
  return torrent::Object();
}

torrent::Object
system_method_set_key(__UNUSED rpc::target_type target, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.empty() || ++args.begin() == args.end())
    throw torrent::input_error("Invalid argument count.");

  rpc::CommandFunctionList* function;
  rpc::CommandMap::iterator itr = rpc::commands.find(args.front().as_string().c_str());

  if (itr == rpc::commands.end() || !(itr->second.m_flags & rpc::CommandMap::flag_modifiable) ||
      (function = dynamic_cast<rpc::CommandFunctionList*>(itr->second.m_variable)) == NULL)
    throw torrent::input_error("Command not modifiable or wrong type.");

  torrent::Object::list_const_iterator itrArgs = ++args.begin();
  const std::string& key = (itrArgs++)->as_string();
  
  if (itrArgs != args.end())
    function->insert(key, system_method_generate_command(itrArgs, args.end()));
  else
    function->erase(key);

  return torrent::Object();
}

torrent::Object
system_method_has_key(__UNUSED rpc::target_type target, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() != 2)
    throw torrent::input_error("Invalid argument count.");

  rpc::CommandFunctionList* function;
  rpc::CommandMap::iterator itr = rpc::commands.find(args.front().as_string().c_str());

  if (itr == rpc::commands.end() ||
      (function = dynamic_cast<rpc::CommandFunctionList*>(itr->second.m_variable)) == NULL)
    throw torrent::input_error("Command is the wrong type.");

  return torrent::Object((int64_t)(function->find(args.back().as_string().c_str()) != function->end()));
}

void
initialize_command_dynamic() {
  CMD_G("system.method.insert",       rak::ptr_fn(&system_method_insert));
  CMD_G_STRING("system.method.erase", rak::ptr_fn(&system_method_erase));
  CMD_G("system.method.set",          rak::ptr_fn(&system_method_set));
  CMD_G("system.method.set_key",      rak::ptr_fn(&system_method_set_key));
  CMD_G("system.method.has_key",      rak::ptr_fn(&system_method_has_key));
}
