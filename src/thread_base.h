#ifndef RTORRENT_UTILS_THREAD_BASE_H
#define RTORRENT_UTILS_THREAD_BASE_H

#include <memory>
#include <pthread.h>
#include <sys/types.h>
#include <torrent/utils/thread.h>

#include "rak/priority_queue_default.h"
#include "core/poll_manager.h"

// Move this class to libtorrent.

class thread_queue_hack;

class ThreadBase : public torrent::utils::Thread {
public:
  typedef rak::priority_queue_default priority_queue;
  typedef void (*thread_base_func)(ThreadBase*);

  ThreadBase();
  virtual ~ThreadBase();

  priority_queue&     task_scheduler() { return m_taskScheduler; }

  // Throw torrent::shutdown_exception to stop the thread.
  void                queue_stop_thread();

  // ATM, only interaction with a thread's allowed by other threads is
  // through the queue_item call.

  // void                queue_item(thread_base_func newFunc);

protected:
  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;

  // TODO: Add thread name.

  // The timer needs to be sync'ed when updated...

  rak::priority_queue_default m_taskScheduler;

  rak::priority_item  m_taskShutdown;

  // Temporary hack to pass messages to a thread. This really needs to
  // be cleaned up and/or integrated into the priority queue itself.
  // std::unique_ptr<thread_queue_hack>  m_threadQueue;
};

#endif
