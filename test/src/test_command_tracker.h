#include "test/helpers/test_fixture.h"

class TestCommandTracker : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandTracker);

  CPPUNIT_TEST(test_dht_override_port_in_range);
  CPPUNIT_TEST(test_dht_override_port_out_of_range);
  CPPUNIT_TEST(test_checked_port_value);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_dht_override_port_in_range();
  void test_dht_override_port_out_of_range();
  void test_checked_port_value();
};
