#include "test/helpers/test_fixture.h"
#include "test/helpers/test_main_thread.h"

#include "rpc/command_map.h"
#include "rpc/jsonrpc.h"

class TestJsonrpc : public test_fixture {
  CPPUNIT_TEST_SUITE(TestJsonrpc);

  CPPUNIT_TEST(test_basics);

  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown();

  void test_basics();

private:
  std::unique_ptr<TestMainThread> m_test_main_thread;

  rpc::JsonRpc m_jsonrpc;
};
