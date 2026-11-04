#include <string>

#include "core/download_list.h"
#include "test/helpers/test_fixture.h"

class TestDownloadList : public test_fixture {
  CPPUNIT_TEST_SUITE(TestDownloadList);

  CPPUNIT_TEST(test_find_hex);
  CPPUNIT_TEST(test_find_hex_wrong_length);
  CPPUNIT_TEST(test_find_hex_after_erase);
  CPPUNIT_TEST(test_insertion_order);
  CPPUNIT_TEST(test_insert_duplicate);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_find_hex();
  void test_find_hex_wrong_length();
  void test_find_hex_after_erase();
  void test_insertion_order();
  void test_insert_duplicate();

private:
  core::DownloadList m_list;
  std::string        m_hex;
};
