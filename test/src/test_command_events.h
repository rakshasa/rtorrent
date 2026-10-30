#include "test/helpers/test_fixture.h"

class TestCommandEvents : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandEvents);

  CPPUNIT_TEST(test_start_tied_skips_erased_downloads);
  CPPUNIT_TEST(test_stop_untied_skips_erased_downloads);
  CPPUNIT_TEST(test_close_untied_skips_erased_downloads);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_start_tied_skips_erased_downloads();
  void test_stop_untied_skips_erased_downloads();
  void test_close_untied_skips_erased_downloads();
};
