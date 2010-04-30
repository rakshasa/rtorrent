#include <cppunit/extensions/HelperMacros.h>

#include "rpc/command.h"

class CommandSlotTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(CommandSlotTest);
  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_type_validity);
  CPPUNIT_TEST(test_convert_return);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() {}
  void tearDown() {}

  void test_basics();
  void test_type_validity();
  void test_convert_return();
};
