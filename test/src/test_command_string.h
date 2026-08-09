#include "test/helpers/test_fixture.h"

class TestCommandString : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandString);

  CPPUNIT_TEST(test_length);
  CPPUNIT_TEST(test_equals);
  CPPUNIT_TEST(test_starts_with);
  CPPUNIT_TEST(test_ends_with);
  CPPUNIT_TEST(test_contains);
  CPPUNIT_TEST(test_substr);
  CPPUNIT_TEST(test_split);
  CPPUNIT_TEST(test_join);
  CPPUNIT_TEST(test_pad);
  CPPUNIT_TEST(test_strip);
  CPPUNIT_TEST(test_map);
  CPPUNIT_TEST(test_replace);
  CPPUNIT_TEST(test_invalid_arguments);
  CPPUNIT_TEST(test_config_syntax);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_length();
  void test_equals();
  void test_starts_with();
  void test_ends_with();
  void test_contains();
  void test_substr();
  void test_split();
  void test_join();
  void test_pad();
  void test_strip();
  void test_map();
  void test_replace();
  void test_invalid_arguments();
  void test_config_syntax();
};
