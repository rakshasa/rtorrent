#include "test/helpers/test_fixture.h"

class TestCommandLocal : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandLocal);

  CPPUNIT_TEST(test_socket_category_commands);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_socket_category_commands();
};
