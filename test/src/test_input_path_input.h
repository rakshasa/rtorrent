#include "test/helpers/test_fixture.h"

#include <string>

class TestInputPathInput : public test_fixture {
  CPPUNIT_TEST_SUITE(TestInputPathInput);

  CPPUNIT_TEST(test_complete_at_end);
  CPPUNIT_TEST(test_complete_mid_line);
  CPPUNIT_TEST(test_cursor_on_separator);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_complete_at_end();
  void test_complete_mid_line();
  void test_cursor_on_separator();

private:
  std::string m_temp_dir;
};
