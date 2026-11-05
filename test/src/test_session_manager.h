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

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_save_waits_for_save_of_same_download();
  void test_flush_waits_for_save_of_same_download();

private:
  std::string                  m_session_dir;
  std::vector<core::Download*> m_downloads;
};
