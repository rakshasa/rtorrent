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

#include <sys/types.h>

#include "core/manager.h"
#include "core/download.h"
#include "core/download_list.h"
#include "core/view.h"
#include "core/view_manager.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"

torrent::Object
cmd_scheduler_simple_added(core::Download* download) {
  unsigned int numActive = (*control->view_manager()->find("active"))->size_visible();
  int64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();
  
  if (numActive < (uint64_t)maxActive)
    control->core()->download_list()->resume(download);

  return torrent::Object();
}

torrent::Object
cmd_scheduler_simple_removed(core::Download* download) {
  control->core()->download_list()->pause(download);

  core::View* viewActive = *control->view_manager()->find("active");
  int64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();

  if ((int64_t)viewActive->size_visible() >= maxActive)
    return torrent::Object();

  // The 'started' view contains all the views we may choose amongst.
  core::View* viewStarted = *control->view_manager()->find("started");

  for (core::View::iterator itr = viewStarted->begin_visible(), last = viewStarted->end_visible(); itr != last; itr++) {
    if ((*itr)->is_active())
      continue;

    control->core()->download_list()->resume(*itr);
  }

  return torrent::Object();
}

torrent::Object
cmd_scheduler_simple_update(core::Download* download) {
  core::View* viewActive = *control->view_manager()->find("active");
  core::View* viewStarted = *control->view_manager()->find("started");

  unsigned int numActive = viewActive->size_visible();
  uint64_t maxActive = rpc::call_command("scheduler.max_active", torrent::Object()).as_value();

  if (viewActive->size_visible() < maxActive) {

    for (core::View::iterator itr = viewStarted->begin_visible(), last = viewStarted->end_visible(); itr != last; itr++) {
      if ((*itr)->is_active())
        continue;

      control->core()->download_list()->resume(*itr);

      if (++numActive >= maxActive)
        break;
    }

  } else if (viewActive->size_visible() > maxActive) {
    
    while (viewActive->size_visible() > maxActive)
      control->core()->download_list()->pause(*viewActive->begin_visible());
  }

  return torrent::Object();
}

void
initialize_command_scheduler() {
  CMD2_VAR_VALUE("scheduler.max_active", int64_t(-1));

  CMD2_DL("scheduler.simple.added",   std::bind(&cmd_scheduler_simple_added, std::placeholders::_1));
  CMD2_DL("scheduler.simple.removed", std::bind(&cmd_scheduler_simple_removed, std::placeholders::_1));
  CMD2_DL("scheduler.simple.update",  std::bind(&cmd_scheduler_simple_update, std::placeholders::_1));
}
