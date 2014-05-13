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

#include <algorithm>
#include <cstdlib>
#include <time.h>
#include <rak/functional.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>

#include "command_scheduler.h"
#include "command_scheduler_item.h"
#include "parse_commands.h"

namespace rpc {

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
  (*itr)->slot() = std::bind(&CommandScheduler::call_item, this, *itr);

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
    rpc::call_object(item->command());

  } catch (torrent::input_error& e) {
    if (m_slotErrorMessage.is_valid())
      m_slotErrorMessage("Scheduled command failed: " + item->key() + ": " + e.what());
  }

  // Still schedule if we caught a torrrent::input_error?
  rak::timer next = item->next_time_scheduled();

  if (next == rak::timer()) {
    // Remove from scheduler?
    return;
  }

  if (next <= cachedTime)
    throw torrent::internal_error("CommandScheduler::call_item(...) tried to schedule a zero interval item.");

  item->enable(next);
}

void
CommandScheduler::parse(const std::string& key,
                        const std::string& bufAbsolute,
                        const std::string& bufInterval,
                        const torrent::Object& command) {
  if (!command.is_string() && !command.is_dict_key())
    throw torrent::bencode_error("Invalid type passed to command scheduler.");

  uint32_t absolute = parse_absolute(bufAbsolute.c_str());
  uint32_t interval = parse_interval(bufInterval.c_str());

  CommandSchedulerItem* item = *insert(key);

  item->command() = command;
  item->set_interval(interval);

  item->enable((cachedTime + rak::timer::from_seconds(absolute)).round_seconds());
}

uint32_t
CommandScheduler::parse_absolute(const char* str) {
  Time result = parse_time(str);
  time_t t;

  // Do the local time thing.
  struct tm local;

  switch (result.first) {
  case 1:
    return result.second;

  case 2:
    t = cachedTime.tval().tv_sec;

    if (localtime_r(&t, &local) == NULL)
      throw torrent::input_error("Could not convert unix time to local time.");

    return (result.second + 3600 - 60 * local.tm_min - local.tm_sec) % 3600;

  case 3:
    t = cachedTime.tval().tv_sec;

    if (localtime_r(&t, &local) == NULL)
      throw torrent::input_error("Could not convert unix time to local time.");

    return (result.second + 24 * 3600 - 3600 * local.tm_hour - 60 * local.tm_min - local.tm_sec) % (24 * 3600);

  case 0:
  default:
    throw torrent::input_error("Could not parse interval.");
  }
}

uint32_t
CommandScheduler::parse_interval(const char* str) {
  Time result = parse_time(str);

  if (result.first == 0)
    throw torrent::input_error("Could not parse interval.");
  
  return result.second;
}

CommandScheduler::Time
CommandScheduler::parse_time(const char* str) {
  Time result(0, 0);

  while (true) {
    char* pos;
    result.first++;
    result.second += strtol(str, &pos, 10);

    if (pos == str || result.second < 0)
      return Time(0, 0);

    while (std::isspace(*pos))
      ++pos;

    if (*pos == '\0')
      return result;

    if (*pos != ':' || result.first > 3)
      return Time(0, 0);

    if (result.first < 3)
      result.second *= 60;
    else
      result.second *= 24;

    str = pos + 1;
  }
}

}
