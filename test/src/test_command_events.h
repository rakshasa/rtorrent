#include "test/helpers/test_fixture.h"

class TestCommandEvents : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandEvents);

  CPPUNIT_TEST(test_start_tied_with_erasing_handlers);
  CPPUNIT_TEST(test_stop_untied_with_erasing_handlers);
  CPPUNIT_TEST(test_close_untied_with_erasing_handlers);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_start_tied_with_erasing_handlers();
  void test_stop_untied_with_erasing_handlers();
  void test_close_untied_with_erasing_handlers();
};
