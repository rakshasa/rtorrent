#ifndef RTORRENT_COMMAND_SCHEDULER_ITEM_H
#define RTORRENT_COMMAND_SCHEDULER_ITEM_H

#include "globals.h"

#include <functional>
#include <torrent/object.h>

namespace rpc {

class CommandSchedulerItem {
public:
  typedef std::function<void ()> slot_void;

  CommandSchedulerItem(const std::string& key) : m_key(key), m_interval(0) {}
  ~CommandSchedulerItem();

  bool                is_queued() const           { return m_task.is_scheduled(); }

  void                enable(rak::timer t);
  void                disable();

  const std::string&  key() const                 { return m_key; }
  torrent::Object&    command()                   { return m_command; }

  // 'interval()' should in the future return some more dynamic values.
  uint32_t            interval() const            { return m_interval; }
  void                set_interval(uint32_t v)    { m_interval = v; }

  rak::timer          time_scheduled() const      { return m_timeScheduled; }
  rak::timer          next_time_scheduled() const;

  slot_void&          slot()                      { return m_task.slot(); }

private:
  CommandSchedulerItem(const CommandSchedulerItem&);
  void operator = (const CommandSchedulerItem&);

  std::string         m_key;
  torrent::Object     m_command;

  uint32_t            m_interval;
  rak::timer          m_timeScheduled;

  rak::priority_item  m_task;

  // Flags for various things.
};

}

#endif
