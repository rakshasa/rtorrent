#include "test/helpers/test_main_thread.h"

class TestView : public test_fixture {
  CPPUNIT_TEST_SUITE(TestView);

  CPPUNIT_TEST(test_filter_dispatches_events_to_the_right_downloads);

  CPPUNIT_TEST(test_filter_skips_erased_snapshot_entries);
  CPPUNIT_TEST(test_sort_preserves_callback_removal_and_focus);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_filter_dispatches_events_to_the_right_downloads();
  void test_filter_skips_erased_snapshot_entries();
  void test_sort_preserves_callback_removal_and_focus();
};
