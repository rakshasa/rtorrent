#include "test/helpers/test_fixture.h"

class TestCommand : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommand);

  CPPUNIT_TEST(test_stack);
  CPPUNIT_TEST(test_stack_double);
  CPPUNIT_TEST(test_is_valid_command);
  CPPUNIT_TEST(test_preparsed_commands);
  CPPUNIT_TEST(test_parse_command_object);
  CPPUNIT_TEST(test_parse_command_multiple_after_each);
  CPPUNIT_TEST(test_call_object_stops_at_erased_download);
  CPPUNIT_TEST(test_call_object_erase_in_last_command);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_stack();
  void test_stack_double();
  void test_is_valid_command();
  void test_preparsed_commands();
  void test_parse_command_object();
  void test_parse_command_multiple_after_each();
  void test_call_object_stops_at_erased_download();
  void test_call_object_erase_in_last_command();
};
