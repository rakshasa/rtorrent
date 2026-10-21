#include "test/helpers/test_main_thread.h"

class TestView : public test_fixture {
  CPPUNIT_TEST_SUITE(TestView);

  CPPUNIT_TEST(test_filter_dispatches_events_to_the_right_downloads);
  CPPUNIT_TEST(test_sort_filter_and_erase);
  CPPUNIT_TEST(test_sort_refuses_view_changes);
  CPPUNIT_TEST(test_filter_refuses_view_changes);
  CPPUNIT_TEST(test_insert_refuses_view_changes);
  CPPUNIT_TEST(test_filter_events_may_change_views);
  CPPUNIT_TEST(test_erase_refuses_added_view);
  CPPUNIT_TEST(test_insert_stops_at_erased_download);
  CPPUNIT_TEST(test_multicall_skips_moved_entries);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_filter_dispatches_events_to_the_right_downloads();
  void test_sort_filter_and_erase();
  void test_sort_refuses_view_changes();
  void test_filter_refuses_view_changes();
  void test_insert_refuses_view_changes();
  void test_filter_events_may_change_views();
  void test_erase_refuses_added_view();
  void test_insert_stops_at_erased_download();
  void test_multicall_skips_moved_entries();
};
