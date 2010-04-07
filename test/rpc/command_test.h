#include <cppunit/extensions/HelperMacros.h>

#include "rpc/command.h"

class CommandTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CommandTest);
  CPPUNIT_TEST(test_stack);
  CPPUNIT_TEST(test_stack_double);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() { }
  void tearDown() {}

  void test_stack();
  void test_stack_double();

private:
};
