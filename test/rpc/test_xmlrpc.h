#include "test/helpers/test_fixture.h"
#include "test/helpers/test_main_thread.h"

#include "rpc/command_map.h"
#include "rpc/xmlrpc.h"

class TestXmlrpc : public test_fixture {
  CPPUNIT_TEST_SUITE(TestXmlrpc);

  CPPUNIT_TEST(test_basics);
  CPPUNIT_TEST(test_invalid_utf8);
  CPPUNIT_TEST(test_size_limit);

  CPPUNIT_TEST_SUITE_END();

public:
  static const int cmd_size = 256;

  void setUp();
  void tearDown();

  void test_basics();
  void test_invalid_utf8();
  void test_size_limit();

private:
  std::unique_ptr<TestMainThread> m_test_main_thread;

  rpc::XmlRpc       m_xmlrpc;
};
