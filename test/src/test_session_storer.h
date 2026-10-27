#include "test/helpers/test_fixture.h"

#include <string>

class TestSessionStorer : public test_fixture {
  CPPUNIT_TEST_SUITE(TestSessionStorer);

  CPPUNIT_TEST(test_temp_file_symlink_is_not_followed);
  CPPUNIT_TEST(test_saved_files_are_owner_only);
  CPPUNIT_TEST(test_saved_files_use_configured_mode);
  CPPUNIT_TEST(test_symlinked_entry_is_not_a_file);
  CPPUNIT_TEST(test_mode_failure_preserves_payload);
  CPPUNIT_TEST(test_mode_failure_still_refuses_storage_errors);
  CPPUNIT_TEST(test_mode_failure_on_each_stream);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_temp_file_symlink_is_not_followed();
  void test_saved_files_are_owner_only();
  void test_saved_files_use_configured_mode();
  void test_symlinked_entry_is_not_a_file();
  void test_mode_failure_preserves_payload();
  void test_mode_failure_still_refuses_storage_errors();
  void test_mode_failure_on_each_stream();

private:
  std::string m_temp_dir;
  std::string m_session_dir;
};
