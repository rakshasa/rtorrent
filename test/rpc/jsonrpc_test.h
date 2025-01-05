#include <cppunit/extensions/HelperMacros.h>

#include "rpc/command_map.h"
#include "rpc/jsonrpc.h"

class JsonrpcTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(JsonrpcTest);
  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp();
  void tearDown() {}

  void test_basics();

private:
  rpc::JsonRpc m_jsonrpc;
};
