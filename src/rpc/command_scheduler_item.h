#ifndef RTORRENT_COMMAND_SCHEDULER_ITEM_H
#define RTORRENT_COMMAND_SCHEDULER_ITEM_H

#include "globals.h"

#include <functional>
#include <torrent/object.h>
#include <torrent/utils/scheduler.h>

namespace rpc {

class CommandSchedulerItem {
public:
  typedef std::function<void ()> slot_void;

  CommandSchedulerItem(const std::string& key) : m_key(key) {}
  ~CommandSchedulerItem();

  bool                is_queued() const           { return m_task.is_scheduled(); }

  void                enable(std::chrono::microseconds t);
  void                disable();

  const std::string&  key() const                 { return m_key; }
  torrent::Object&    command()                   { return m_command; }

  // 'interval()' should in the future return some more dynamic values.
  uint32_t            interval() const            { return m_interval; }
  void                set_interval(uint32_t v)    { m_interval = v; }

  std::chrono::microseconds time_scheduled() const { return m_time_scheduled; }
  std::chrono::microseconds next_time_scheduled() const;

  slot_void&          slot()                      { return m_task.slot(); }

private:
  CommandSchedulerItem(const CommandSchedulerItem&);
  void operator = (const CommandSchedulerItem&);

  std::string         m_key;
  torrent::Object     m_command;

  uint32_t                  m_interval{};
  std::chrono::microseconds m_time_scheduled;

  torrent::utils::SchedulerEntry m_task;

  // Flags for various things.
};

}

#endif
