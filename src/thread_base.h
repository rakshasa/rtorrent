#ifndef RTORRENT_UTILS_THREAD_BASE_H
#define RTORRENT_UTILS_THREAD_BASE_H

#include <torrent/utils/thread.h>

class ThreadBase : public torrent::utils::Thread {
public:
  ThreadBase();

protected:
  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;
};

#endif
