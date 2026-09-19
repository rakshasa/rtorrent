#include "test/helpers/test_main_thread.h"

class TestView : public TestFixtureWithMainThread {
  CPPUNIT_TEST_SUITE(TestView);

  CPPUNIT_TEST(test_filter_dispatches_events_to_the_right_downloads);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_filter_dispatches_events_to_the_right_downloads();
};
