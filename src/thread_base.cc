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

class thread_queue_hack : private std::vector<ThreadBase::thread_base_func> {
public:
  static constexpr unsigned int max_size = 32;

  using value_type = ThreadBase::thread_base_func;
  using base_type  = std::vector<value_type>;

  thread_queue_hack() { reserve(max_size); };

  bool empty() {
    std::lock_guard<std::mutex> lguard(m_lock);
    return base_type::empty();
  }

  void push_back(value_type v) {
    const std::lock_guard<std::mutex> lguard(m_lock);
    if (base_type::size() >= max_size)
      throw torrent::internal_error("Overflowed thread_queue max size of " + std::to_string(max_size) + ".");
    base_type::push_back(v);
  }

  void copy_and_clear(base_type& target) {
    std::lock_guard<std::mutex> lguard(m_lock);
    target.assign(begin(), end());
    base_type::clear();
  }

private:
  std::mutex m_lock;
};

void
throw_shutdown_exception() { throw torrent::shutdown_exception(); }

ThreadBase::ThreadBase() {
  m_taskShutdown.slot() = std::bind(&throw_shutdown_exception);

  m_threadQueue = std::make_unique<thread_queue_hack>();
}

// Defined in here and not the header so that the default destructor
// can properly deduce how to destruct the unique_ptr<thread_queue_hack>
ThreadBase::~ThreadBase() = default;

// Move to libtorrent...
void
ThreadBase::stop_thread(ThreadBase* thread) {
  if (!thread->m_taskShutdown.is_queued())
    priority_queue_insert(&thread->m_taskScheduler, &thread->m_taskShutdown, cachedTime);
}

int64_t
ThreadBase::next_timeout_usec() {
  if (m_taskScheduler.empty())
    return rak::timer::from_seconds(600).usec();
  else if (m_taskScheduler.top()->time() <= cachedTime)
    return 0;
  else
    return (m_taskScheduler.top()->time() - cachedTime).usec();
}

void
ThreadBase::call_queued_items() {
  std::vector<thread_queue_hack::value_type> queue;
  m_threadQueue->copy_and_clear(queue);
  for (auto itr : queue) {
    itr(this);
  }
}

void
ThreadBase::call_events() {
  // Check for new queued items set by other threads.
  if (!m_threadQueue->empty())
    call_queued_items();

  rak::priority_queue_perform(&m_taskScheduler, cachedTime);

  process_callbacks();
}

void
ThreadBase::queue_item(thread_base_func newFunc) {
  m_threadQueue->push_back(newFunc);

  // Make it also restart inactive threads?
  if (m_state == STATE_ACTIVE)
    interrupt();
}
