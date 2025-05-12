#include "test/helpers/test_fixture.h"

class TestCommandSlot : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandSlot);

  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_type_validity);
  CPPUNIT_TEST(test_convert_return);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_basics();
  void test_type_validity();
  void test_convert_return();
};
