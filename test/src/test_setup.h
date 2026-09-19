#include "test/helpers/test_fixture.h"

class TestSetup : public test_fixture {
  CPPUNIT_TEST_SUITE(TestSetup);

  CPPUNIT_TEST(test_config_comment_log_add_output);
  CPPUNIT_TEST(test_config_comment_log_add_output_no_args);
  CPPUNIT_TEST(test_config_comment_log_add_output_one_arg);
  CPPUNIT_TEST(test_config_comment_log_add_output_too_many_args);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_config_comment_log_add_output();
  void test_config_comment_log_add_output_no_args();
  void test_config_comment_log_add_output_one_arg();
  void test_config_comment_log_add_output_too_many_args();
};
