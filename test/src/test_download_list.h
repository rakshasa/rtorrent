#include <string>

#include "core/download_list.h"
#include "test/helpers/test_fixture.h"

class TestDownloadList : public test_fixture {
  CPPUNIT_TEST_SUITE(TestDownloadList);

  CPPUNIT_TEST(test_find_hex);
  CPPUNIT_TEST(test_find_hex_wrong_length);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_find_hex();
  void test_find_hex_wrong_length();

private:
  core::DownloadList m_list;
  std::string        m_hex;
};
