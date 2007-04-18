// rTorrent - BitTorrent client
// Copyright (C) 2006, Jari Sundell
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

#include "utils/variable_map.h"

namespace utils {
  class CommandSlot;
  class CommandVariable;
}

// By using a static array we avoid allocating the variables on the
// heap. This should reduce memory use and improve cache locality.
#define COMMAND_SLOTS_SIZE     18
#define COMMAND_VARIABLES_SIZE 23

extern utils::CommandSlot      commandSlots[COMMAND_SLOTS_SIZE];
extern utils::CommandSlot*     commandSlotsItr;
extern utils::CommandVariable  commandVariables[COMMAND_VARIABLES_SIZE];
extern utils::CommandVariable* commandVariablesItr;

void initialize_variables();
void initialize_download_variables();
void initialize_command_events();
void initialize_command_ui();

void initialize_commands();

void
add_variable(const char* getKey, const char* setKey, const char* defaultSetKey,
             utils::VariableMap::generic_slot getSlot, utils::VariableMap::generic_slot setSlot,
             const torrent::Object& defaultObject);

#define ADD_VARIABLE_BOOL(key, defaultValue) \
add_variable("get_" key, "set_" key, key, &utils::CommandVariable::get_bool, &utils::CommandVariable::set_bool, (int64_t)defaultValue);

#define ADD_VARIABLE_VALUE(key, defaultValue) \
add_variable("get_" key, "set_" key, key, &utils::CommandVariable::get_value, &utils::CommandVariable::set_value, (int64_t)defaultValue);

#define ADD_VARIABLE_STRING(key, defaultValue) \
add_variable("get_" key, "set_" key, key, &utils::CommandVariable::get_string, &utils::CommandVariable::set_string, std::string(defaultValue));

#define ADD_COMMAND_SLOT(key, function, slot) \
  commandSlotsItr->set_slot(slot); \
  variables->insert(key, commandSlotsItr++, &utils::CommandSlot::function, utils::VariableMap::flag_dont_delete);

#endif
