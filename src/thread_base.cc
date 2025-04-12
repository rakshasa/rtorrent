#include "config.h"

#include "thread_base.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <rak/error_number.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/utils/log.h>
#include <unistd.h>

#include "globals.h"

void
throw_shutdown_exception() { throw torrent::shutdown_exception(); }

ThreadBase::ThreadBase() {
  m_taskShutdown.slot() = std::bind(&throw_shutdown_exception);
}

// Defined in here and not the header so that the default destructor
// can properly deduce how to destruct the unique_ptr<thread_queue_hack>
ThreadBase::~ThreadBase() = default;

// Move to libtorrent...
void
ThreadBase::queue_stop_thread() {
  if (!m_taskShutdown.is_queued())
    priority_queue_insert(&m_taskScheduler, &m_taskShutdown, cachedTime);
}

int64_t
ThreadBase::next_timeout_usec() {
  if (m_taskScheduler.empty())
    return rak::timer::from_seconds(600).usec();
  else if (m_taskScheduler.top()->time() <= cachedTime)
    return 0;
  else
    return (m_taskScheduler.top()->time() - cachedTime).usec();

  // TODO: Thread-specific cachedTime.

  // if (!taskScheduler.empty())
  //   return std::max(taskScheduler.top()->time() - cachedTime, rak::timer()).usec();
  // else
  //   return rak::timer::from_seconds(600).usec();
}

void
ThreadBase::call_events() {
  rak::priority_queue_perform(&m_taskScheduler, cachedTime);

  process_callbacks();
}
