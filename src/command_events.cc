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

#include <sigc++/bind.h>

#include "core/download_list.h"
#include "core/manager.h"
#include "utils/command_slot.h"
#include "utils/parse.h"
#include "utils/variable_map.h"

#include "globals.h"
#include "control.h"

void
apply_on_state_change(core::DownloadList::slot_map* slotMap, const torrent::Object& rawArgs) {
  const torrent::Object::list_type& args = rawArgs.as_list();

  if (args.size() < 2)
    throw torrent::input_error("Too few arguments.");

  if (args.front().as_string().empty())
    throw torrent::input_error("Empty key.");

  std::string key = "1_state_" + args.front().as_string();

  if (args.back().as_string().empty())
    slotMap->erase(key);
  else
    (*slotMap)[key] = sigc::bind(sigc::mem_fun(control->download_variables(), &utils::VariableMap::process_d_std_single),
                                 utils::convert_list_to_command(++args.begin(), args.end()));
}

#define ADD_COMMAND_SLOT(key, function, slot) \
variables->insert(key, new utils::CommandSlot(slot), &utils::CommandSlot::function);

void
initialize_command_events() {
  utils::VariableMap* variables = control->variable();
  core::DownloadList* downloadList = control->core()->download_list();

  ADD_COMMAND_SLOT("on_insert",       call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_insert()));
  ADD_COMMAND_SLOT("on_erase",        call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_erase()));
  ADD_COMMAND_SLOT("on_open",         call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_open()));
  ADD_COMMAND_SLOT("on_close",        call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_close()));
  ADD_COMMAND_SLOT("on_start",        call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_start()));
  ADD_COMMAND_SLOT("on_stop",         call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_stop()));
  ADD_COMMAND_SLOT("on_hash_queued",  call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_hash_queued()));
  ADD_COMMAND_SLOT("on_hash_removed", call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_hash_removed()));
  ADD_COMMAND_SLOT("on_hash_done",    call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_hash_done()));
  ADD_COMMAND_SLOT("on_finished",     call_list, rak::bind_ptr_fn(&apply_on_state_change, &downloadList->slot_map_finished()));
}
