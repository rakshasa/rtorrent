#include <cppunit/extensions/HelperMacros.h>

class TestParseOptions : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(TestParseOptions);
  CPPUNIT_TEST(test_flag_basic);
  CPPUNIT_TEST(test_flag_error);

  CPPUNIT_TEST(test_flags_basic);
  CPPUNIT_TEST(test_flags_error);
  CPPUNIT_TEST(test_flags_complex);

  CPPUNIT_TEST(test_flags_print_vector);
  CPPUNIT_TEST(test_flags_print_flags);

  CPPUNIT_TEST(test_flag_libtorrent);
  CPPUNIT_TEST(test_flags_libtorrent);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp() { }
  void tearDown() {}

  void test_flag_basic();
  void test_flag_error();

  void test_flags_basic();
  void test_flags_error();
  void test_flags_complex();

  void test_flags_print_vector();
  void test_flags_print_flags();

  void test_flag_libtorrent();
  void test_flags_libtorrent();
};
