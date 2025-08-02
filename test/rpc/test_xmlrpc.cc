#include "config.h"

#include "test/rpc/test_xmlrpc.h"

#include <string>

#include "control.h"
#include "globals.h"
#include "command_helpers.h"
#include "rpc/command_map.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestXmlrpc);

torrent::Object
xmlrpc_cmd_test_reflect([[maybe_unused]] rpc::target_type t, const torrent::Object& obj) {
  return obj;
}

torrent::Object
xmlrpc_cmd_test_reflect_string([[maybe_unused]] rpc::target_type t, const std::string& obj) {
  return obj;
}

void initialize_command_dynamic();

#if defined(HAVE_XMLRPC_TINYXML2) && !defined(HAVE_XMLRPC_C)

std::vector<std::tuple<std::string, std::string, std::string>> basic_requests = {
  std::make_tuple("Basic call",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data/></array></value></param></params></methodResponse>"),

  std::make_tuple("Basic call without params",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data/></array></value></param></params></methodResponse>"),

  std::make_tuple("UTF-8 string",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><string>чао</string></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><string>чао</string></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("emoji string",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><string>😊</string></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><string>😊</string></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("base64 data (which gets returned as a string)",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><base64>Zm9vYmFy</base64></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><string>foobar</string></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("i8 ints",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><i8>41</i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><i8>41</i8></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("i8 ints",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><i8>2247483647</i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><i8>2247483647</i8></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("negative i8 ints",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><i8>-2347483647</i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><i8>-2347483647</i8></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("Simple array",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><array><data><value><i8>2247483647</i8></value></data></array></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><array><data><value><i8>2247483647</i8></value></data></array></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("Empty array",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><array><data></data></array></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><array><data/></array></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("Empty struct",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><struct></struct></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><struct/></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("Simple struct",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><struct><member><name>lowerBound</name><value><i8>18</i8></value></member><member><name>upperBound</name><value><i8>139</i8></value></member></struct></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><struct><member><name>lowerBound</name><value><i8>18</i8></value></member><member><name>upperBound</name><value><i8>139</i8></value></member></struct></value></data></array></value></param></params></methodResponse>"),

  std::make_tuple("Invalid - missing method",
                  "<?xml version=\"1.0\"?><methodCall><methodName>no_such_method</methodName><params><param><value><i8>41</i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-506</i8></value></member><member><name>faultString</name><value><string>method 'no_such_method' not defined</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("Invalid - i8 target",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><i8>41</i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-500</i8></value></member><member><name>faultString</name><value><string>invalid parameters: target must be a string</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("Invalid - empty int tag",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><i8/></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-501</i8></value></member><member><name>faultString</name><value><string>unable to parse empty integer</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("Invalid - empty int text",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><i8></i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-501</i8></value></member><member><name>faultString</name><value><string>unable to parse empty integer</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("Invalid - broken XML",
                  "thodCall><methodName>test_a</methodName><params><param><value><i8>41</i8></value></param></params></method",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-503</i8></value></member><member><name>faultString</name><value><string>Error=XML_ERROR_PARSING_ELEMENT ErrorID=6 (0x6) Line number=1: XMLElement name=method</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("Invalid - non-integer i8",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><i8>string value</i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-501</i8></value></member><member><name>faultString</name><value><string>unable to parse integer value</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("Invalid - float i8",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><i8>3.14</i8></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-501</i8></value></member><member><name>faultString</name><value><string>unable to parse integer value</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("Invalid - non-boolean boolean",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><boolean>string value</boolean></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-501</i8></value></member><member><name>faultString</name><value><string>unknown boolean value: string value</string></value></member></struct></value></fault></methodResponse>"),

  std::make_tuple("CMD2_ANY_STRING",
                  "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect_string</methodName><params><param><value><string></string></value></param><param><value><string>test</string></value></param></params></methodCall>",
                  "<?xml version=\"1.0\"?><methodResponse><params><param><value><string>test</string></value></param></params></methodResponse>")

};

void
TestXmlrpc::setUp() {
  m_test_main_thread = TestMainThread::create();
  m_test_main_thread->init_thread();

  m_xmlrpc = rpc::XmlRpc();
  m_xmlrpc.initialize();
  setlocale(LC_ALL, "");
  control = new Control;

  if (rpc::commands.find("xmlrpc_reflect") == rpc::commands.end())
    CMD2_ANY("xmlrpc_reflect", &xmlrpc_cmd_test_reflect);

  if (rpc::commands.find("xmlrpc_reflect_string") == rpc::commands.end())
    CMD2_ANY_STRING("xmlrpc_reflect_string", &xmlrpc_cmd_test_reflect_string);
}

void
TestXmlrpc::tearDown() {
  m_test_main_thread.reset();
}

void
TestXmlrpc::test_basics() {
  for (auto& test : basic_requests) {
    std::string output;
    m_xmlrpc.process(std::get<1>(test).c_str(), std::get<1>(test).size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
    CPPUNIT_ASSERT_EQUAL_MESSAGE(std::get<0>(test), std::get<2>(test), output);
  }
}

void
TestXmlrpc::test_invalid_utf8() {
  // Surprisingly, this call doesn't fail. TinyXML-2 technically expects
  // valid UTF-8, but doesn't check strings, and Object strings are
  // just a series of bytes so it reflects just fine.
  std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><string>\xc3\x28</string></value></param></params></methodCall>";
  std::string expected = "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><string>\xc3\x28</string></value></data></array></value></param></params></methodResponse>";
  std::string output;
  m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
  CPPUNIT_ASSERT_EQUAL(expected, output);
}

void
TestXmlrpc::test_size_limit() {
  std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><string>\xc3\x28</string></value></param></params></methodCall>";
  std::string expected = "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-509</i8></value></member><member><name>faultString</name><value><string>Content size exceeds maximum XML-RPC limit</string></value></member></struct></value></fault></methodResponse>";
  std::string output;
  m_xmlrpc.set_size_limit(1);
  m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
  CPPUNIT_ASSERT_EQUAL(expected, output);
}

#else

void TestXmlrpc::test_invalid_utf8() {}
void TestXmlrpc::test_basics() {}
void TestXmlrpc::test_size_limit() {}
void TestXmlrpc::setUp() {}
void TestXmlrpc::tearDown() {}

#endif
