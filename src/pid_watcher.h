#ifndef RTORRENT_PID_WATCHER_H
#define RTORRENT_PID_WATCHER_H

#include <torrent/common.h>

struct atomic_queue {
  std::array<std::atomic<pid_t>, 16> queue{};


class PidWatcher {
public:
  PidWatcher(std::thread::id thread_id);
  ~PidWatcher();

  void                prepare_spawn();
  void                completed_spawn(pid_t pid);

private:
  std::thread::id     m_thread_id;

  align_cacheline

  std::atomic<bool>   m_spawn_in_progress{};

};

#endif
