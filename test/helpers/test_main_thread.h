#ifndef TEST_HELPERS_TEST_MAIN_THREAD_H
#define TEST_HELPERS_TEST_MAIN_THREAD_H

#include <memory>

#include "test/helpers/test_fixture.h"
#include "test/helpers/test_thread.h"
#include "torrent/common.h"
#include "torrent/utils/thread.h"

class TestMainThread : public torrent::utils::Thread {
public:
  static std::unique_ptr<TestMainThread> create();
  static std::unique_ptr<TestMainThread> create_with_mock();

  const char*         name() const override  { return "rtorrent test main"; }

  void                init_thread() override;
  void                cleanup_thread() override;

  void                test_set_cached_time(std::chrono::microseconds t) { set_cached_time(365 * 24h + t); }
  void                test_add_cached_time(std::chrono::microseconds t) { set_cached_time(cached_time() + t); }
  void                test_process_events_without_cached_time()         { process_events_without_cached_time(); }

private:
  TestMainThread() = default;

  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;
};

class TestFixtureWithMainThread : public test_fixture {
public:
  void setUp();
  void tearDown();

  std::unique_ptr<TestMainThread> m_main_thread;
};

class TestFixtureWithMainAndDiskThread : public test_fixture {
public:
  void setUp();
  void tearDown();

  std::unique_ptr<TestMainThread> m_main_thread;
};

class TestFixtureWithMainAndTrackerThread : public test_fixture {
public:
  void setUp();
  void tearDown();

  std::unique_ptr<TestMainThread> m_main_thread;
};

class TestFixtureWithMockAndMainThread : public test_fixture {
public:
  void setUp();
  void tearDown();

  std::unique_ptr<TestMainThread> m_main_thread;
};

#endif // TEST_HELPERS_TEST_MAIN_THREAD_H
