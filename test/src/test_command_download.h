#include "core/download_list.h"
#include "test/helpers/test_fixture.h"

class TestCommandDownload : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandDownload);

  CPPUNIT_TEST(test_name_or_base64);
  CPPUNIT_TEST(test_name_or_as_binary);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_name_or_base64();
  void test_name_or_as_binary();

private:
  core::DownloadList m_list;
};
