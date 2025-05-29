#include "test/helpers/test_fixture.h"

#include "rpc/command_map.h"

class TestCommandMap : public test_fixture {
  CPPUNIT_TEST_SUITE(TestCommandMap);

  CPPUNIT_TEST(test_basics);

  CPPUNIT_TEST_SUITE_END();

public:
  static const int cmd_size = 256;

  void setUp() { m_commandItr = m_commands; }

  void test_basics();

private:
  rpc::CommandMap m_map;

  rpc::command_base m_commands[cmd_size];
  rpc::command_base* m_commandItr;
};
