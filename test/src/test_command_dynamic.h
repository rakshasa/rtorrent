#include "test/helpers/test_fixture.h"
#include "test/helpers/test_main_thread.h"

class TestCommandDynamic : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandDynamic);

  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_get_set);
  CPPUNIT_TEST(test_old_style);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_basics();
  void test_get_set();

  void test_old_style();

private:
  std::unique_ptr<TestMainThread> m_test_main_thread;
};
