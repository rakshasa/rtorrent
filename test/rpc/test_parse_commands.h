#include "test/helpers/test_main_thread.h"

#include "torrent/utils/log_buffer.h"

class TestParseCommands : public TestFixtureWithMainThread {
  CPPUNIT_TEST_SUITE(TestParseCommands);

  CPPUNIT_TEST(test_nothrow_logs_the_error);
  CPPUNIT_TEST(test_object_and_download_wrappers_log);
  CPPUNIT_TEST(test_burst_is_limited_and_counted);
  CPPUNIT_TEST(test_error_after_limit_is_logged_again);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_nothrow_logs_the_error();
  void test_object_and_download_wrappers_log();
  void test_burst_is_limited_and_counted();
  void test_error_after_limit_is_logged_again();

private:
  torrent::log_buffer_ptr m_log;
};
