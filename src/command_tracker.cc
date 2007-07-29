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

#include <rak/error_number.h>
#include <torrent/tracker.h>

#include "core/manager.h"
#include "rpc/command_slot.h"
#include "rpc/command_tracker_slot.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

void
apply_t_set_enabled(torrent::Tracker* tracker, int64_t state) {
  if (state)
    tracker->enable();
  else
    tracker->disable();
}

#define ADD_CT_SLOT(key, function, slot, parm, doc)    \
  commandTrackerSlotsItr->set_slot(slot); \
  rpc::commands.insert_tracker(key, commandTrackerSlotsItr++, &rpc::CommandTrackerSlot::function, rpc::CommandMap::flag_dont_delete, parm, doc);

#define ADD_CT_SLOT_PUBLIC(key, function, slot, parm, doc)    \
  commandTrackerSlotsItr->set_slot(slot); \
  rpc::commands.insert_tracker(key, commandTrackerSlotsItr++, &rpc::CommandTrackerSlot::function, rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, parm, doc);

#define ADD_CT_VOID(key, slot) \
  ADD_CT_SLOT_PUBLIC("get_t_" key, call_unknown, rpc::object_t_fn(slot), "i:", "")

#define ADD_CT_VALUE_UNI(key, get) \
  ADD_CT_SLOT_PUBLIC("get_t_" key, call_unknown, rpc::object_void_t_fn(get), "i:", "")

#define ADD_CT_VALUE_BI(key, set, get) \
  ADD_CT_SLOT_PUBLIC("set_t_" key, call_value, rpc::object_value_t_fn(set), "i:i", "") \
  ADD_CT_SLOT_PUBLIC("get_t_" key, call_unknown, rpc::object_void_t_fn(get), "i:", "")

#define ADD_CT_STRING_UNI(key, get) \
  ADD_CT_SLOT_PUBLIC("get_t_" key, call_unknown, rpc::object_void_t_fn(get), "s:", "")

void
initialize_command_tracker() {
  ADD_CT_STRING_UNI("url",              std::mem_fun(&torrent::Tracker::url));
  ADD_CT_VALUE_UNI("group",             std::mem_fun(&torrent::Tracker::group));
  ADD_CT_VALUE_UNI("type",              std::mem_fun(&torrent::Tracker::tracker_type));
  ADD_CT_STRING_UNI("id",               std::mem_fun(&torrent::Tracker::tracker_id));

  ADD_CT_VALUE_BI("enabled",            std::ptr_fun(&apply_t_set_enabled), std::mem_fun(&torrent::Tracker::is_enabled));

  ADD_CT_VALUE_UNI("is_open",           std::mem_fun(&torrent::Tracker::is_open));

  ADD_CT_VALUE_UNI("normal_interval",   std::mem_fun(&torrent::Tracker::normal_interval));
  ADD_CT_VALUE_UNI("min_interval",      std::mem_fun(&torrent::Tracker::min_interval));

  ADD_CT_VALUE_UNI("scrape_time_last",  std::mem_fun(&torrent::Tracker::scrape_time_last));
  ADD_CT_VALUE_UNI("scrape_complete",   std::mem_fun(&torrent::Tracker::scrape_complete));
  ADD_CT_VALUE_UNI("scrape_incomplete", std::mem_fun(&torrent::Tracker::scrape_incomplete));
  ADD_CT_VALUE_UNI("scrape_downloaded", std::mem_fun(&torrent::Tracker::scrape_downloaded));
}
