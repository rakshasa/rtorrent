#include "test/helpers/test_fixture.h"

class TestUiDownloadList : public test_fixture {
  CPPUNIT_TEST_SUITE(TestUiDownloadList);

  CPPUNIT_TEST(test_filter_pattern);
  CPPUNIT_TEST(test_filter_command);
  CPPUNIT_TEST(test_filter_command_does_not_inject);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_filter_pattern();
  void test_filter_command();
  void test_filter_command_does_not_inject();
};
