#include "test/helpers/test_fixture.h"

#include <string>
#include <vector>

namespace core {
class Download;
}

class TestSessionManager : public test_fixture {
  CPPUNIT_TEST_SUITE(TestSessionManager);

  CPPUNIT_TEST(test_save_waits_for_save_of_same_download);
  CPPUNIT_TEST(test_flush_waits_for_save_of_same_download);
  CPPUNIT_TEST(test_remove_waits_for_matching_save);
  CPPUNIT_TEST(test_remove_does_not_wait_for_other_save);
  CPPUNIT_TEST(test_remove_cancels_pending_build);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_save_waits_for_save_of_same_download();
  void test_flush_waits_for_save_of_same_download();
  void test_remove_waits_for_matching_save();
  void test_remove_does_not_wait_for_other_save();
  void test_remove_cancels_pending_build();

private:
  bool                         remove_waits_for_save_of(core::Download* saving);

  std::string                  m_session_dir;
  std::vector<core::Download*> m_downloads;
};
