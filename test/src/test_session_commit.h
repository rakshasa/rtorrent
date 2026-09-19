#include "test/helpers/test_fixture.h"

#include <string>

class TestSessionCommit : public test_fixture {
  CPPUNIT_TEST_SUITE(TestSessionCommit);

  CPPUNIT_TEST(test_commit_publishes_all_three_files);
  CPPUNIT_TEST(test_commit_with_fsync_publishes_all_three_files);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_commit_publishes_all_three_files();
  void test_commit_with_fsync_publishes_all_three_files();

private:
  void commit_and_verify(bool use_fsyncdisk);

  std::string m_session_dir;
};
