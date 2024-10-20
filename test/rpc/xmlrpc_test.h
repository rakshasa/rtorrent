#include <cppunit/extensions/HelperMacros.h>

#include "rpc/command_map.h"
#include "rpc/xmlrpc.h"

class XmlrpcTest : public CppUnit::TestFixture {
  CPPUNIT_TEST_SUITE(XmlrpcTest);
  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_invalid_utf8);
  CPPUNIT_TEST_SUITE_END();

public:
  static const int cmd_size = 256;

  void setUp();
  void tearDown() {}

  void test_basics();
  void test_invalid_utf8();

private:
  rpc::XmlRpc m_xmlrpc;
  
  rpc::CommandMap m_map;

  rpc::command_base m_commands[cmd_size];
  rpc::command_base* m_commandItr;
};
