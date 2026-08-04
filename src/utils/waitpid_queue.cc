#include "config.h"

#include "utils/waitpid_queue.h"

#include <sys/wait.h>
#include <torrent/exceptions.h>

namespace utils {

WaitpidQueue::WaitpidQueue() {
  m_worker = std::async(std::launch::async, [this]() {
      bool is_running = true;
      auto wait_time  = 50ms;

      while (is_running) {
        if (!m_queue.empty()) {
          auto start_time = std::chrono::steady_clock::now();

          while (std::chrono::steady_clock::now() - start_time < wait_time) {
            if (m_should_shutdown.load(std::memory_order_acquire))
              return;

            std::this_thread::sleep_for(50ms);

            if (m_wakeup_worker.load(std::memory_order_acquire))
              break;
          }

        } else {
          m_wakeup_worker.wait(false, std::memory_order_acquire);
        }

        // Adds a small delay to allow new processes to finish if they're quickly spawned and
        // terminated.
        std::this_thread::sleep_for(50ms);

        std::set<pid_t> queue;

        {
          std::lock_guard<std::mutex> guard(m_mutex);

          if (m_should_shutdown)
            return;

          if (m_queue.empty())
            throw torrent::internal_error("WaitpidQueue worker thread woke up but queue is empty.");

          queue = m_queue;

          m_wakeup_worker.store(false, std::memory_order_release);
        }

        wait_time = std::min(10 * 1000ms, wait_time * 2);

        for (auto pid : queue) {
          if (::waitpid(pid, nullptr, WNOHANG) == 0)
            continue;

          {
            std::lock_guard<std::mutex> guard(m_mutex);

            if (m_queue.erase(pid) != 1)
              throw torrent::internal_error("WaitpidQueue worker thread could not find pid in queue.");
          }

          wait_time = std::max(50ms, wait_time / 2);

          m_remaining.fetch_sub(1, std::memory_order_release);
          m_remaining.notify_all();
        }
      }
    });
}

WaitpidQueue::~WaitpidQueue() {
  {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_should_shutdown = true;
  }

  m_wakeup_worker.store(true, std::memory_order_release);
  m_wakeup_worker.notify_all();

  // m_worker.wait();
}

void
WaitpidQueue::close_pid(pid_t pid) {
  if (pid < 0)
    throw torrent::internal_error("WaitpidQueue::close_pid() called with invalid pid.");

  m_remaining.fetch_add(1, std::memory_order_acquire);

  {
    std::lock_guard<std::mutex> guard(m_mutex);

    // if (!m_queue.empty()) {
    //   m_queue.push_back(pid);
    //   return;
    // }

    m_queue.insert(pid);
  }

  m_wakeup_worker.store(true, std::memory_order_release);
  m_wakeup_worker.notify_all();
}

void
WaitpidQueue::wait_for(uint32_t max_remaining) {
  while (m_remaining.load(std::memory_order_acquire) > max_remaining)
    m_remaining.wait(max_remaining, std::memory_order_acquire);
}

} // namespace torrent::utils
