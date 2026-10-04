#include "test/helpers/test_fixture.h"

class TestParse : public test_fixture {
  CPPUNIT_TEST_SUITE(TestParse);

  CPPUNIT_TEST(test_whole_value_in_range);
  CPPUNIT_TEST(test_whole_value_out_of_range);
  CPPUNIT_TEST(test_whole_value_bases);
  CPPUNIT_TEST(test_whole_value_prefixes);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_whole_value_in_range();
  void test_whole_value_out_of_range();
  void test_whole_value_bases();
  void test_whole_value_prefixes();
};
