#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

// ipv4_range_parse is a pure function, so this does not use test_fixture and
// the mock and logging setup that comes with it.
class TestCommandIp : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TestCommandIp);

  CPPUNIT_TEST(test_single_address);
  CPPUNIT_TEST(test_explicit_range);

  CPPUNIT_TEST(test_cidr);
  CPPUNIT_TEST(test_cidr_zero_mask);
  CPPUNIT_TEST(test_cidr_full_mask);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_single_address();
  void test_explicit_range();

  void test_cidr();
  void test_cidr_zero_mask();
  void test_cidr_full_mask();
};
