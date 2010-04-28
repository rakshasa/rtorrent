#include <cppunit/extensions/HelperMacros.h>

class CommandDynamicTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CommandDynamicTest);
  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_get_set);
  CPPUNIT_TEST(test_old_style);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown() {}

  void test_basics();
  void test_get_set();

  void test_old_style();

private:
};
