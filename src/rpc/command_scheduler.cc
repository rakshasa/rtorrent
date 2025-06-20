#include "config.h"

#include "rpc/command_scheduler.h"

#include <algorithm>
#include <cstdlib>
#include <time.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/utils/chrono.h>

#include "rpc/command_scheduler_item.h"
#include "rpc/parse_commands.h"

namespace rpc {

CommandScheduler::~CommandScheduler() {
  for (auto item : *this)
    delete item;
}

CommandScheduler::iterator
CommandScheduler::find(const std::string& key) {
  return std::find_if(begin(), end(), [key](CommandSchedulerItem* item) { return key == item->key(); });
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
    if (m_slotErrorMessage)
      m_slotErrorMessage("Scheduled command failed: " + item->key() + ": " + e.what());
  }

  // Still schedule if we caught a torrrent::input_error?
  auto next = item->next_time_scheduled();

  if (next == std::chrono::microseconds(0)) {
    // Remove from scheduler?
    return;
  }

  if (next <= torrent::this_thread::cached_time())
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

  item->enable(torrent::utils::ceil_seconds(torrent::this_thread::cached_time() + std::chrono::seconds(absolute)));
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
    t = torrent::this_thread::cached_seconds().count();

    if (localtime_r(&t, &local) == NULL)
      throw torrent::input_error("Could not convert unix time to local time.");

    return (result.second + 3600 - 60 * local.tm_min - local.tm_sec) % 3600;

  case 3:
    t = torrent::this_thread::cached_seconds().count();

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
