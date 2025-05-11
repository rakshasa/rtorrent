#include "test/helpers/test_fixture.h"

class TestCommand : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommand);

  CPPUNIT_TEST(test_stack);
  CPPUNIT_TEST(test_stack_double);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_stack();
  void test_stack_double();
};
