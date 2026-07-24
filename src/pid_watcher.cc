#include "config.h"

#include "pid_watcher.h"

#include <cassert>
#include <torrent/exceptions.h>

PidWatcher::PidWatcher(std::thread::id thread_id)
  : m_thread_id(thread_id) {
}

PidWatcher::~PidWatcher() = default;

void
PidWatcher::prepare_spawn() {
  assert(std::this_thread::get_id() == m_thread_id);

  if (m_spawn_in_progress.exchange(true, std::memory_order_acquire))
    throw torrent::internal_error("PidWatcher::prepare_spawn() called while spawn already in progress.");
}
