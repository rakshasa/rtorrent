#include "test/helpers/test_fixture.h"

class TestTrustScope : public test_fixture {
  CPPUNIT_TEST_SUITE(TestTrustScope);

  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_nested);
  CPPUNIT_TEST(test_exception);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_basics();
  void test_nested();
  void test_exception();
};
