#include "test/helpers/test_fixture.h"

#include "rpc/rpc_manager.h"

class TestRpcManager : public test_fixture {
  CPPUNIT_TEST_SUITE(TestRpcManager);

  CPPUNIT_TEST(test_size_limit_bounds);

  CPPUNIT_TEST_SUITE_END();

public:
  void test_size_limit_bounds();

private:
  rpc::RpcManager m_rpc_manager;
};
