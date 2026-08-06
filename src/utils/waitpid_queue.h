#ifndef RTORRENT_UTILS_WAITPID_QUEUE_H
#define RTORRENT_UTILS_WAITPID_QUEUE_H

#include <future>
#include <set>
#include <torrent/system/common.h>

namespace utils {

class WaitpidQueue {
public:
  WaitpidQueue();
  ~WaitpidQueue();

  uint32_t            size() const;

  void                close_pid(int pid);

  void                wait_for(uint32_t max_remaining);

  // TODO: Add a signal handler to tell the worker thread to wake up. Use a counter to batch wakeups.

private:
  WaitpidQueue(const WaitpidQueue&) = delete;
  WaitpidQueue& operator=(const WaitpidQueue&) = delete;

  std::future<void>   m_worker;

  align_cacheline

  std::mutex          m_mutex;
  std::set<pid_t>     m_queue;

  align_cacheline

  std::atomic<bool>     m_wakeup_worker{};
  std::atomic<bool>     m_should_shutdown{};

  std::atomic<uint32_t> m_remaining{};
};

inline uint32_t WaitpidQueue::size() const { return m_remaining.load(std::memory_order_acquire); }

} // namespace utils

#endif
