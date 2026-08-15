#include "test/helpers/test_main_thread.h"

class TestCommandScheduler : public TestFixtureWithMainThread {
  CPPUNIT_TEST_SUITE(TestCommandScheduler);

  CPPUNIT_TEST(test_parse_rearms_existing_key);
  CPPUNIT_TEST(test_find_locates_a_scheduled_key);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_parse_rearms_existing_key();
  void test_find_locates_a_scheduled_key();
};
