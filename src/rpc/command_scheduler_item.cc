#include "config.h"

#include "rpc/command_scheduler_item.h"

#include <torrent/exceptions.h>
#include <torrent/utils/chrono.h>

namespace rpc {

CommandSchedulerItem::~CommandSchedulerItem() {
  torrent::this_thread::scheduler()->erase(&m_task);
}

void
CommandSchedulerItem::enable(std::chrono::microseconds t) {
  if (t == std::chrono::microseconds())
    throw torrent::internal_error("CommandSchedulerItem::enable() t == 0.");

  if (is_queued())
    disable();

  // If 'first' is zero then we execute the task
  // immediately. ''interval()'' will not return zero so we never end
  // up in an infinit loop.
  m_time_scheduled = t;

  torrent::this_thread::scheduler()->wait_until(&m_task, t);
}

void
CommandSchedulerItem::disable() {
  m_time_scheduled = std::chrono::microseconds();
  torrent::this_thread::scheduler()->erase(&m_task);
}

std::chrono::microseconds
CommandSchedulerItem::next_time_scheduled() const {
  if (m_interval == 0)
    return std::chrono::microseconds();

  if (m_time_scheduled == std::chrono::microseconds())
    throw torrent::internal_error("CommandSchedulerItem::next_time_scheduled() m_time_scheduled == 0.");

  auto next = m_time_scheduled;

  // This should be done in a non-looping manner.
  do {
    next += std::chrono::seconds(m_interval);
  } while (next <= torrent::utils::ceil_seconds(torrent::this_thread::cached_time()));

  return next;
}

}
