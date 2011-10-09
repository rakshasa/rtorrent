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

#include <vector>
#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/data/file_list_iterator.h>

// Get better logging...
#include "globals.h"
#include "control.h"
#include "core/manager.h"

#include "command.h"
#include "command_map.h"

// For XMLRPC stuff, clean up.
#include "xmlrpc.h"
#include "parse_commands.h"

namespace rpc {

command_base::stack_type command_base::current_stack;

CommandMap::~CommandMap() {
  std::vector<const char*> keys;

  for (iterator itr = base_type::begin(), last = base_type::end(); itr != last; itr++) {
//     if (!(itr->second.m_flags & flag_dont_delete))
//       delete itr->second.m_variable;

    if (itr->second.m_flags & flag_delete_key)
      keys.push_back(itr->first);
  }

  for (std::vector<const char*>::iterator itr = keys.begin(), last = keys.end(); itr != last; itr++)
    delete [] *itr;
}

CommandMap::iterator
CommandMap::insert(key_type key, int flags, const char* parm, const char* doc) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("CommandMap::insert(...) tried to insert an already existing key.");

  // TODO: This is not honoring the public_xmlrpc flags!!!
  if (rpc::xmlrpc.is_valid() && (flags & flag_public_xmlrpc))
  // if (rpc::xmlrpc.is_valid())
    rpc::xmlrpc.insert_command(key, parm, doc);

  return base_type::insert(itr, value_type(key, command_map_data_type(flags, parm, doc)));
}

// void
// CommandMap::insert(key_type key, const command_map_data_type src) {
//   iterator itr = base_type::find(key);

//   if (itr != base_type::end())
//     throw torrent::internal_error("CommandMap::insert(...) tried to insert an already existing key.");

//   itr = base_type::insert(itr, value_type(key, command_map_data_type(src.m_variable, src.m_flags | flag_dont_delete, src.m_parm, src.m_doc)));

//   // We can assume all the slots are the same size.
//   itr->second.m_anySlot = src.m_anySlot;
// }

void
CommandMap::erase(iterator itr) {
  if (itr == end())
    return;

  // TODO: Remove the redirects instead...
  if (itr->second.m_flags & flag_has_redirects)
    throw torrent::input_error("Can't erase a command that has redirects.");

//   if (!(itr->second.m_flags & flag_dont_delete))
//     delete itr->second.m_variable;

  const char* key = itr->second.m_flags & flag_delete_key ? itr->first : NULL;

  base_type::erase(itr);
  delete [] key;
}

void
CommandMap::create_redirect(key_type key_new, key_type key_dest, int flags) {
  iterator new_itr  = base_type::find(key_new);
  iterator dest_itr = base_type::find(key_dest);

  if (dest_itr == base_type::end())
    throw torrent::input_error("Tried to redirect to a key that doesn't exist: '" + std::string(key_dest) + "'.");
  
  if (new_itr != base_type::end())
    throw torrent::input_error("Tried to create a redirect key that already exists: '" + std::string(key_new) + "'.");
  
  if (dest_itr->second.m_flags & flag_is_redirect)
    throw torrent::input_error("Tried to redirect to a key that is not marked 'flag_is_redirect': '" +
                               std::string(key_dest) + "'.");

  dest_itr->second.m_flags |= flag_has_redirects;

  flags |= dest_itr->second.m_flags & ~(flag_delete_key | flag_has_redirects | flag_public_xmlrpc);

  // TODO: This is not honoring the public_xmlrpc flags!!!
  if (rpc::xmlrpc.is_valid() && (flags & flag_public_xmlrpc))
    rpc::xmlrpc.insert_command(key_new, dest_itr->second.m_parm, dest_itr->second.m_doc);

  iterator itr = base_type::insert(base_type::end(),
                                   value_type(key_new, command_map_data_type(flags,
                                                                             dest_itr->second.m_parm,
                                                                             dest_itr->second.m_doc)));

  // We can assume all the slots are the same size.
  itr->second.m_variable = dest_itr->second.m_variable;
  itr->second.m_anySlot = dest_itr->second.m_anySlot;
}

const CommandMap::mapped_type
CommandMap::call_catch(key_type key, target_type target, const mapped_type& args, const char* err) {
  try {
    return call_command(key, args, target);
  } catch (torrent::input_error& e) {
    control->core()->push_log((err + std::string(e.what())).c_str());
    return torrent::Object();
  }
}

const CommandMap::mapped_type
CommandMap::call_command(key_type key, const mapped_type& arg, target_type target) {
  iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Command \"" + std::string(key) + "\" does not exist.");

  return itr->second.m_anySlot(&itr->second.m_variable, target, arg);
}

const CommandMap::mapped_type
CommandMap::call_command(iterator itr, const mapped_type& arg, target_type target) {
  return itr->second.m_anySlot(&itr->second.m_variable, target, arg);
}

}
