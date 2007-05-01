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

#include "config.h"

#include <torrent/exceptions.h>

#include "utils/command_slot.h"
#include "utils/command_variable.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

utils::CommandSlot      commandSlots[COMMAND_SLOTS_SIZE];
utils::CommandSlot*     commandSlotsItr = commandSlots;
utils::CommandVariable  commandVariables[COMMAND_VARIABLES_SIZE];
utils::CommandVariable* commandVariablesItr = commandVariables;

void initialize_variables();
void initialize_download_variables();
void initialize_command_events();
void initialize_command_local();
void initialize_command_network();
void initialize_command_ui();

void
initialize_commands() {
  initialize_variables();
  initialize_download_variables();
  initialize_command_events();
  initialize_command_network();
  initialize_command_local();
  initialize_command_ui();

#ifdef ADDING_COMMANDS 
  if (commandSlotsItr > commandSlots + COMMAND_SLOTS_SIZE ||
      commandVariablesItr > commandVariables + COMMAND_VARIABLES_SIZE)
#else
  if (commandSlotsItr != commandSlots + COMMAND_SLOTS_SIZE ||
      commandVariablesItr != commandVariables + COMMAND_VARIABLES_SIZE)
#endif
    throw torrent::internal_error("initialize_commands() static command array size mismatch.");
}

void
add_variable(const char* getKey, const char* setKey, const char* defaultSetKey,
             utils::VariableMap::generic_slot getSlot, utils::VariableMap::generic_slot setSlot,
             const torrent::Object& defaultObject) {
  utils::CommandVariable* variable = commandVariablesItr++;
  variable->set_variable(defaultObject);

  control->variable()->insert(getKey, variable, getSlot, utils::VariableMap::flag_dont_delete);
  control->variable()->insert(setKey, variable, setSlot, utils::VariableMap::flag_dont_delete);

  if (defaultSetKey)
    control->variable()->insert(defaultSetKey, variable, setSlot, utils::VariableMap::flag_dont_delete);
}
