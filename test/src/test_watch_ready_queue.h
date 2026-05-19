#include "test/helpers/test_main_thread.h"

class TestWatchReadyQueue : public TestFixtureWithMainThread {
  CPPUNIT_TEST_SUITE(TestWatchReadyQueue);

  CPPUNIT_TEST(test_repeated_events_reset_quiet_time);
  CPPUNIT_TEST(test_unrelated_events_do_not_delay_missing_retry);
  CPPUNIT_TEST(test_missing_paths_expire_without_dispatch);
  CPPUNIT_TEST(test_shutdown_discards_pending_loads);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_repeated_events_reset_quiet_time();
  void test_unrelated_events_do_not_delay_missing_retry();
  void test_missing_paths_expire_without_dispatch();
  void test_shutdown_discards_pending_loads();
};
