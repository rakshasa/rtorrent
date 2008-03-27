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

#ifndef RTORRENT_UTILS_COMMAND_HELPERS_H
#define RTORRENT_UTILS_COMMAND_HELPERS_H

#include "rpc/command_slot.h"
#include "rpc/parse_commands.h"

namespace rpc {
  class CommandVariable;
}

// By using a static array we avoid allocating the variables on the
// heap. This should reduce memory use and improve cache locality.
#define COMMAND_SLOTS_SIZE          200
#define COMMAND_VARIABLES_SIZE      100
#define COMMAND_DOWNLOAD_SLOTS_SIZE 150
#define COMMAND_FILE_SLOTS_SIZE     30
#define COMMAND_FILE_ITR_SLOTS_SIZE 10
#define COMMAND_PEER_SLOTS_SIZE     20
#define COMMAND_TRACKER_SLOTS_SIZE  15
#define COMMAND_ANY_SLOTS_SIZE      50

#define ADDING_COMMANDS

extern rpc::CommandSlot<void>    commandSlots[COMMAND_SLOTS_SIZE];
extern rpc::CommandSlot<void>*   commandSlotsItr;
extern rpc::CommandVariable      commandVariables[COMMAND_VARIABLES_SIZE];
extern rpc::CommandVariable*     commandVariablesItr;
extern rpc::CommandSlot<core::Download*>             commandDownloadSlots[COMMAND_DOWNLOAD_SLOTS_SIZE];
extern rpc::CommandSlot<core::Download*>*            commandDownloadSlotsItr;
extern rpc::CommandSlot<torrent::File*>              commandFileSlots[COMMAND_FILE_SLOTS_SIZE];
extern rpc::CommandSlot<torrent::File*>*             commandFileSlotsItr;
extern rpc::CommandSlot<torrent::FileListIterator*>  commandFileItrSlots[COMMAND_FILE_ITR_SLOTS_SIZE];
extern rpc::CommandSlot<torrent::FileListIterator*>* commandFileItrSlotsItr;
extern rpc::CommandSlot<torrent::Peer*>              commandPeerSlots[COMMAND_PEER_SLOTS_SIZE];
extern rpc::CommandSlot<torrent::Peer*>*             commandPeerSlotsItr;
extern rpc::CommandSlot<torrent::Tracker*>           commandTrackerSlots[COMMAND_TRACKER_SLOTS_SIZE];
extern rpc::CommandSlot<torrent::Tracker*>*          commandTrackerSlotsItr;
extern rpc::CommandSlot<rpc::target_type>            commandAnySlots[COMMAND_ANY_SLOTS_SIZE];
extern rpc::CommandSlot<rpc::target_type>*           commandAnySlotsItr;

void initialize_commands();

void
add_variable(const char* getKey, const char* setKey, const char* defaultSetKey,
             rpc::Command::generic_slot getSlot, rpc::Command::generic_slot setSlot,
             const torrent::Object& defaultObject);

#define ADD_VARIABLE_BOOL(key, defaultValue) \
add_variable("get_" key, "set_" key, key, &rpc::CommandVariable::get_bool, &rpc::CommandVariable::set_bool, (int64_t)defaultValue);

#define ADD_VARIABLE_VALUE(key, defaultValue) \
add_variable("get_" key, "set_" key, key, &rpc::CommandVariable::get_value, &rpc::CommandVariable::set_value, (int64_t)defaultValue);

#define ADD_VARIABLE_STRING(key, defaultValue) \
add_variable("get_" key, "set_" key, key, &rpc::CommandVariable::get_string, &rpc::CommandVariable::set_string, std::string(defaultValue));

#define ADD_C_STRING(key, defaultValue) \
add_variable(key, NULL, NULL, &rpc::CommandVariable::get_string, NULL, std::string(defaultValue));

#define ADD_COMMAND_SLOT(key, function, slot, parm, doc)    \
  commandSlotsItr->set_slot(slot); \
  rpc::commands.insert_type(key, commandSlotsItr++, &rpc::CommandSlot<void>::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_ANY_SLOT(key, function, slot, parm, doc)    \
  commandAnySlotsItr->set_slot(slot); \
  rpc::commands.insert_type(key, commandAnySlotsItr++, &rpc::CommandSlot<rpc::target_type>::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_COMMAND_SLOT_PRIVATE(key, function, slot) \
  commandSlotsItr->set_slot(slot); \
  rpc::commands.insert_type(key, commandSlotsItr++, &rpc::CommandSlot<void>::function, rpc::CommandMap::flag_dont_delete, NULL, NULL);

#define ADD_COMMAND_COPY(key, function, parm, doc) \
  rpc::commands.insert_type(key, (commandSlotsItr - 1), &rpc::CommandSlot<void>::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_COMMAND_COPY_PRIVATE(key, function) \
  rpc::commands.insert_type(key, (commandSlotsItr - 1), &rpc::CommandSlot<void>::function, rpc::CommandMap::flag_dont_delete, NULL, NULL);

#define ADD_COMMAND_VALUE_TRI(key, set, get) \
  ADD_COMMAND_SLOT_PRIVATE(key, call_value, rpc::object_value_fn(set))      \
  ADD_COMMAND_COPY("set_" key,  call_value, "i:i", "")                      \
  ADD_COMMAND_SLOT("get_" key,  call_unknown, rpc::object_void_fn(get), "i:", "")

#define ADD_COMMAND_VALUE_TRI_KB(key, set, get) \
  ADD_COMMAND_SLOT_PRIVATE(key, call_value_kb, rpc::object_value_fn(set)) \
  ADD_COMMAND_COPY("set_" key,  call_value, "i:i", "")                      \
  ADD_COMMAND_SLOT("get_" key,  call_unknown, rpc::object_void_fn(get), "i:", "")

#define ADD_COMMAND_VALUE_SET_OCT(prefix, key, set)                 \
  ADD_COMMAND_SLOT_PRIVATE(key, call_value_oct, rpc::object_value_fn(set)) \
  ADD_COMMAND_COPY(prefix "set_" key,  call_value, "i:i", "")

#define ADD_COMMAND_VALUE_TRI_OCT(prefix, key, set, get)                 \
  ADD_COMMAND_SLOT_PRIVATE(key, call_value_oct, rpc::object_value_fn(set)) \
  ADD_COMMAND_COPY(prefix "set_" key,  call_value, "i:i", "")                      \
  ADD_COMMAND_SLOT(prefix "get_" key,  call_unknown, rpc::object_void_fn(get), "i:", "")

#define ADD_COMMAND_STRING_PREFIX(prefix, key, set, get) \
  ADD_COMMAND_SLOT_PRIVATE(key, call_string, rpc::object_string_fn(set))      \
  ADD_COMMAND_COPY(prefix "set_" key,  call_string, "i:s", "") \
  ADD_COMMAND_SLOT(prefix "get_" key,  call_unknown, rpc::object_void_fn(get), "s:", "")

#define ADD_COMMAND_STRING_TRI(key, set, get)                    \
  ADD_COMMAND_STRING_PREFIX("", key, set, get)

#define ADD_COMMAND_VOID(key, slot) \
  ADD_COMMAND_SLOT(key, call_unknown, rpc::object_void_fn(slot), "i:", "")

#define ADD_COMMAND_VALUE(key, slot) \
  ADD_COMMAND_SLOT(key, call_value, slot, "i:i", "")

#define ADD_COMMAND_VALUE_UN(key, slot) \
  ADD_COMMAND_SLOT(key, call_value, rpc::object_value_fn(slot), "i:i", "")

#define ADD_COMMAND_STRING(key, slot) \
  ADD_COMMAND_SLOT(key, call_string, slot, "i:s", "")

#define ADD_COMMAND_STRING_UN(key, slot) \
  ADD_COMMAND_SLOT(key, call_string, rpc::object_string_fn(slot), "i:s", "")

#define ADD_COMMAND_LIST(key, slot) \
  ADD_COMMAND_SLOT(key, call_list, slot, "i:", "")

#define ADD_COMMAND_NONE(key, slot) \
  ADD_COMMAND_SLOT(key, call_unknown, slot, "i:", "")

#define ADD_ANY_NONE(key, slot) \
  ADD_ANY_SLOT(key, call_unknown, slot, "i:", "")

#define ADD_ANY_VALUE(key, slot) \
  ADD_ANY_SLOT(key, call_value, slot, "i:i", "")

#define ADD_ANY_LIST(key, slot) \
  ADD_ANY_SLOT(key, call_list, slot, "i:i", "")

#define ADD_COMMAND_NONE_L(key, slot) \
  ADD_COMMAND_SLOT(key, call_unknown, slot, "A:", "")

//
// DOWNLOAD RELATED COMMANDS
//

#define CMD_D_SLOT(key, function, slot, parm, doc)    \
  commandDownloadSlotsItr->set_slot(slot); \
  rpc::commands.insert_type(key, commandDownloadSlotsItr++, &rpc::CommandSlot<core::Download*>::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define CMD_D_ANY(key, slot) \
  CMD_D_SLOT(key, call_unknown, slot, "i:", "")

#define CMD_D_STRING(key, slot) \
  CMD_D_SLOT(key, call_string, slot, "i:", "")

#define CMD_D_VOID(key, slot) \
  CMD_D_SLOT(key, call_unknown, rpc::object_fn(slot), "i:", "")

#endif
