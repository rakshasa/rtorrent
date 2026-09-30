#include "test/helpers/test_fixture.h"

class TestCommandGroups : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandGroups);

  CPPUNIT_TEST(test_max_unchoked_in_range);
  CPPUNIT_TEST(test_max_unchoked_out_of_range);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_max_unchoked_in_range();
  void test_max_unchoked_out_of_range();
};
