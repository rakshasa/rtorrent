#ifndef TEST_HELPERS_TEST_MAIN_THREAD_H
#define TEST_HELPERS_TEST_MAIN_THREAD_H

#include <memory>
#include <torrent/common.h>
#include <torrent/utils/thread.h>

#include "test/helpers/test_thread.h"

class TestMainThread : public torrent::utils::Thread {
public:
  static std::unique_ptr<TestMainThread> create();

  ~TestMainThread() override;

  const char*         name() const override  { return "rtorrent test main"; }

  void                init_thread() override;

  void                test_set_cached_time(std::chrono::microseconds t) { set_cached_time(365 * 24h + t); }
  void                test_process_events_without_cached_time()         { process_events_without_cached_time(); }

private:
  TestMainThread();

  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;
};

#endif // TEST_HELPERS_TEST_MAIN_THREAD_H
