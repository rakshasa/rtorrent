#include "test/helpers/test_fixture.h"

class TestCommandEvents : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandEvents);

  CPPUNIT_TEST(test_start_tied_with_erasing_handlers);
  CPPUNIT_TEST(test_stop_untied_with_erasing_handlers);
  CPPUNIT_TEST(test_close_untied_with_erasing_handlers);

  CPPUNIT_TEST(test_hash_failure_releases_active);
  CPPUNIT_TEST(test_hash_failure_lets_scheduler_start_another);
  CPPUNIT_TEST(test_hash_failure_with_erasing_handler);

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
};
