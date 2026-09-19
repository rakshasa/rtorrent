#include "test/helpers/test_fixture.h"

class TestCommandThrottle : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandThrottle);

  CPPUNIT_TEST(test_global_rate_in_range);
  CPPUNIT_TEST(test_global_rate_kb_out_of_range);
  CPPUNIT_TEST(test_global_rate_bytes_out_of_range);
  CPPUNIT_TEST(test_global_rate_negative);
  CPPUNIT_TEST(test_named_rate_in_range);
  CPPUNIT_TEST(test_named_rate_out_of_range);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_global_rate_in_range();
  void test_global_rate_kb_out_of_range();
  void test_global_rate_bytes_out_of_range();
  void test_global_rate_negative();
  void test_named_rate_in_range();
  void test_named_rate_out_of_range();
};
