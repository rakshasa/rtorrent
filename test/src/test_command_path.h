#include "test/helpers/test_fixture.h"

#include <string>

class TestCommandPath : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandPath);

  CPPUNIT_TEST(test_resolves_symlink);
  CPPUNIT_TEST(test_removes_relative_components);
  CPPUNIT_TEST(test_expands_tilde);
  CPPUNIT_TEST(test_missing_path_is_empty);
  CPPUNIT_TEST(test_missing_path_throws);
  CPPUNIT_TEST(test_commands);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_resolves_symlink();
  void test_removes_relative_components();
  void test_expands_tilde();
  void test_missing_path_is_empty();
  void test_missing_path_throws();
  void test_commands();

private:
  std::string m_temp_dir;
};
