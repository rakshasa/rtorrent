#include <cppunit/extensions/HelperMacros.h>

class TestParseOptions : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TestParseOptions);
  CPPUNIT_TEST(test_basic);
  CPPUNIT_TEST(test_errors);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() { }
  void tearDown() {}

  void test_basic();
  void test_errors();

private:
};
