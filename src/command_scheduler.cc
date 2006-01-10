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

#include <algorithm>
#include <rak/functional.h>
#include <torrent/exceptions.h>

#include "command_scheduler.h"
#include "command_scheduler_item.h"

CommandScheduler::~CommandScheduler() {
  std::for_each(begin(), end(), rak::call_delete<CommandSchedulerItem>());
}

CommandScheduler::iterator
CommandScheduler::find(const std::string& key) {
  return std::find_if(begin(), end(), rak::equal(key, std::mem_fun(&CommandSchedulerItem::key)));
}

CommandScheduler::iterator
CommandScheduler::insert(const std::string& key) {
  if (key.empty())
    throw torrent::input_error("Scheduler received an empty key.");

  iterator itr = find(key);

  if (itr == end())
    itr = base_type::insert(end(), NULL);
  else
    delete *itr;

  *itr = new CommandSchedulerItem(key);
  (*itr)->set_slot(rak::bind_mem_fn(this, &CommandScheduler::call_item, *itr));

  return itr;
}

void
CommandScheduler::erase(iterator itr) {
  if (itr == end())
    return;

  delete *itr;
  base_type::erase(itr);
}

void
CommandScheduler::call_item(value_type item) {
  if (item->is_queued())
    throw torrent::internal_error("CommandScheduler::call_item(...) called but item is still queued.");

  if (std::find(begin(), end(), item) == end())
    throw torrent::internal_error("CommandScheduler::call_item(...) called but the item isn't in the scheduler.");

  // Remove the item before calling the command if it should be
  // removed.

  try {
    m_slotCommand(item->command());

  } catch (torrent::input_error& e) {
    if (m_slotErrorMessage.is_valid())
      m_slotErrorMessage("Scheduled command failed: " + item->key() + ": " + e.what());
  }

  uint32_t interval = item->interval();

  // Enable if we caught a torrrent::input_error?
  if (interval != 0)
    item->enable(interval);
}
