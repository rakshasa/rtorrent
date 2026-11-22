#include "test/helpers/test_fixture.h"

class TestCommandEvents : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandEvents);

  CPPUNIT_TEST(test_start_tied_with_erasing_handlers);
  CPPUNIT_TEST(test_stop_untied_with_erasing_handlers);
  CPPUNIT_TEST(test_close_untied_with_erasing_handlers);

  CPPUNIT_TEST(test_hash_failure_releases_active);
  CPPUNIT_TEST(test_hash_failure_lets_scheduler_start_another);
  CPPUNIT_TEST(test_hash_failure_with_erasing_handler);

  CPPUNIT_TEST(test_resume_with_erasing_handler);
  CPPUNIT_TEST(test_resume_with_erasing_and_failing_handler);
  CPPUNIT_TEST(test_pause_with_erasing_handler);
  CPPUNIT_TEST(test_pause_with_erasing_and_failing_handler);
  CPPUNIT_TEST(test_close_directly_with_erasing_handler);
  CPPUNIT_TEST(test_close_quick_with_erasing_handler);
  CPPUNIT_TEST(test_shutdown_with_erasing_handlers);
  CPPUNIT_TEST(test_close_directly_open_with_erasing_handler);
  CPPUNIT_TEST(test_quick_shutdown_with_erasing_handlers);
  CPPUNIT_TEST(test_clear_refuses_erasing_handlers);
  CPPUNIT_TEST(test_clear_keeps_downloads_for_handlers);
  CPPUNIT_TEST(test_clear_closes_the_rest_after_failed_close);
  CPPUNIT_TEST(test_quick_shutdown_closes_open_download);
  CPPUNIT_TEST(test_hashing_pass_stops_after_view_change);
  CPPUNIT_TEST(test_hashing_pass_stops_after_reallocated_view);
  CPPUNIT_TEST(test_hashing_pass_stops_after_same_size_view_change);
  CPPUNIT_TEST(test_hashing_pass_logs_failure_after_view_change);
  CPPUNIT_TEST(test_hash_done_stops_after_stopped_handler_erases);
  CPPUNIT_TEST(test_meta_download_stops_after_stop_handler_erases);
  CPPUNIT_TEST(test_meta_download_stops_after_close_handler_erases);
  CPPUNIT_TEST(test_check_hash_logs_after_erasing_and_failing_handler);

  CPPUNIT_TEST(test_close_keeps_queued_hash_check);
  CPPUNIT_TEST(test_close_keeps_queued_hash_check_of_closed_download);
  CPPUNIT_TEST(test_close_stops_running_hash_check);
  CPPUNIT_TEST(test_close_clears_stopped_hash_queue);
  CPPUNIT_TEST(test_close_clears_failed_hash_queue);
  CPPUNIT_TEST(test_stop_removes_queued_hash_check);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_start_tied_with_erasing_handlers();
  void test_stop_untied_with_erasing_handlers();
  void test_close_untied_with_erasing_handlers();
  void test_hash_failure_releases_active();
  void test_hash_failure_lets_scheduler_start_another();
  void test_hash_failure_with_erasing_handler();
  void test_resume_with_erasing_handler();
  void test_resume_with_erasing_and_failing_handler();
  void test_pause_with_erasing_handler();
  void test_pause_with_erasing_and_failing_handler();
  void test_close_directly_with_erasing_handler();
  void test_close_quick_with_erasing_handler();
  void test_shutdown_with_erasing_handlers();
  void test_close_directly_open_with_erasing_handler();
  void test_quick_shutdown_with_erasing_handlers();
  void test_clear_refuses_erasing_handlers();
  void test_clear_keeps_downloads_for_handlers();
  void test_clear_closes_the_rest_after_failed_close();
  void test_quick_shutdown_closes_open_download();
  void test_hashing_pass_stops_after_view_change();
  void test_hashing_pass_stops_after_reallocated_view();
  void test_hashing_pass_stops_after_same_size_view_change();
  void test_hashing_pass_logs_failure_after_view_change();
  void test_hash_done_stops_after_stopped_handler_erases();
  void test_meta_download_stops_after_stop_handler_erases();
  void test_meta_download_stops_after_close_handler_erases();
  void test_check_hash_logs_after_erasing_and_failing_handler();
  void test_close_keeps_queued_hash_check();
  void test_close_keeps_queued_hash_check_of_closed_download();
  void test_close_stops_running_hash_check();
  void test_close_clears_stopped_hash_queue();
  void test_close_clears_failed_hash_queue();
  void test_stop_removes_queued_hash_check();
};
