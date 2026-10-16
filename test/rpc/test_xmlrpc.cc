#include "config.h"

#include "test/rpc/test_xmlrpc.h"

#include <string>

#include "control.h"
#include "globals.h"
#include "command_helpers.h"
#include "rpc/command_map.h"
#include "rpc/scgi_task.h"
#include <torrent/utils/string_manip.h>

CPPUNIT_TEST_SUITE_REGISTRATION(TestXmlrpc);

torrent::Object
xmlrpc_cmd_test_reflect([[maybe_unused]] rpc::target_type t, const torrent::Object& obj) {
  return obj;
}

torrent::Object
xmlrpc_cmd_test_reflect_string([[maybe_unused]] rpc::target_type t, const std::string& obj) {
  return obj;
}

torrent::Object
xmlrpc_cmd_test_map([[maybe_unused]] rpc::target_type t, const std::string& key) {
  auto result = torrent::Object::create_map();
  result.as_map()["partial-safe-key"] = "safe";
  result.as_map()[key] = "value";
  return result;
}

torrent::Object
xmlrpc_cmd_test_oversized([[maybe_unused]] rpc::target_type t, [[maybe_unused]] const torrent::Object& obj) {
  return torrent::Object(std::string(rpc::SCgiTask::max_response_size + (1 << 20), 'a'));
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

  if (rpc::commands.find("xmlrpc_map") == rpc::commands.end())
    CMD2_ANY_STRING("xmlrpc_map", &xmlrpc_cmd_test_map);

  if (rpc::commands.find("xmlrpc_oversized") == rpc::commands.end())
    CMD2_ANY("xmlrpc_oversized", &xmlrpc_cmd_test_oversized);
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
  // TinyXML-2 accepts these input bytes, but the response must be valid XML.
  std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><string>\xc3\x28</string></value></param></params></methodCall>";
  std::string expected = "<?xml version=\"1.0\"?><methodResponse><params><param><value><array><data><value><base64>wyg=</base64></value></data></array></value></param></params></methodResponse>";
  std::string output;
  m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
  CPPUNIT_ASSERT_EQUAL(expected, output);
}

void
TestXmlrpc::test_xml_string_bytes() {
  auto reflect = [this](const std::string& raw) {
    const auto input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect_string</methodName>"
      "<params><param><value><string></string></value></param><param><value><base64>" +
      torrent::utils::transform_to_base64(raw) + "</base64></value></param></params></methodCall>";
    std::string output;
    m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
    return output;
  };
  auto expect = [&reflect](const std::string& raw, const std::string& value) {
    CPPUNIT_ASSERT_EQUAL("<?xml version=\"1.0\"?><methodResponse><params><param><value>" + value +
                         "</value></param></params></methodResponse>", reflect(raw));
  };
  auto binary = [&expect](const std::string& raw) {
    expect(raw, "<base64>" + torrent::utils::transform_to_base64(raw) + "</base64>");
  };

  // A control byte in a metainfo name must survive strict XML decoding.
  binary("ctl\x01name.bin");
  for (unsigned char ch = 0; ch < 0x20; ++ch) {
    const auto raw = std::string("before") + static_cast<char>(ch) + "after";
    if (ch == '\t' || ch == '\n')
      expect(raw, "<string>" + raw + "</string>");
    else
      binary(raw);
  }

  for (const auto& raw : {std::string("\xc3\x28"), std::string("\x80"), std::string("\xe2\x82"),
                          std::string("\xc0\x80"), std::string("\xc1\xbf"), std::string("\xe0\x80\x80"),
                          std::string("\xed\xa0\x80"), std::string("\xf0\x80\x80\x80"),
                          std::string("\xf4\x90\x80\x80"), std::string("\xf5\x80\x80\x80"),
                          std::string("\xef\xbf\xbe"), std::string("\xef\xbf\xbf")})
    binary(raw);

  expect("", "<string></string>");
  expect("normal", "<string>normal</string>");
  expect("<&>\"'", "<string>&lt;&amp;&gt;\"'</string>");
  const std::string valid = "чао😊\x7f\xc2\x80\xed\x9f\xbf\xee\x80\x80\xef\xbf\xbd\xf4\x8f\xbf\xbf";
  expect(valid, "<string>" + valid + "</string>");
}

void
TestXmlrpc::test_xml_fault_bytes() {
  // The missing method's name is echoed in a top-level fault, not a result.
  const std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>bad\x01method</methodName></methodCall>";
  std::string output;
  m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
  CPPUNIT_ASSERT(output.find("<name>faultCode</name><value><i8>-506</i8>") != std::string::npos);
  CPPUNIT_ASSERT(output.find("<name>faultString</name><value><string>escaped bytes: method 'bad\\x01method' not defined</string>") != std::string::npos);
}

void
TestXmlrpc::test_xml_member_name() {
  const std::string expected = "<?xml version=\"1.0\"?><methodResponse><fault><value><struct>"
    "<member><name>faultCode</name><value><i8>-501</i8></value></member>"
    "<member><name>faultString</name><value><string>struct member name is not XML 1.0 text</string></value></member>"
    "</struct></value></fault></methodResponse>";
  for (const auto& key : {std::string("unsafe\x01key"), std::string("nul\0key", 7), std::string("cr\rkey"),
                          std::string("\xc0\x80"), std::string("\xef\xbf\xbf")}) {
    const std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_map</methodName>"
      "<params><param><value><string></string></value></param><param><value><base64>" +
      torrent::utils::transform_to_base64(key) + "</base64></value></param></params></methodCall>";
    std::string output;
    unsigned writes = 0;
    m_xmlrpc.process(input.c_str(), input.size(), [&output, &writes](const char* c, uint32_t l) {
      ++writes;
      output.append(c, l);
      return true;
    });
    // A partly printed result must never reach the writer before the fault.
    CPPUNIT_ASSERT_EQUAL(1u, writes);
    CPPUNIT_ASSERT_EQUAL(expected, output);
  }
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

// A command whose result does not fit in the SCGI response buffer must be
// answered with a fault, not handed to the writer. SCgiTask::receive_write
// treats an oversized body as an internal_error, which is not caught by any
// RPC handler and terminates the process.
void
TestXmlrpc::test_response_size_limit() {
  std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_oversized</methodName><params><param><value><string></string></value></param></params></methodCall>";
  std::string expected = "<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><i8>-509</i8></value></member><member><name>faultString</name><value><string>Response size exceeds maximum XML-RPC limit</string></value></member></struct></value></fault></methodResponse>";
  std::string output;

  m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});

  CPPUNIT_ASSERT_MESSAGE("response handed to the writer is " + std::to_string(output.size()) +
                         " bytes, over the " + std::to_string(rpc::SCgiTask::max_response_size) + " byte SCGI limit",
                         output.size() <= rpc::SCgiTask::max_response_size);
  CPPUNIT_ASSERT_EQUAL(expected, output);
}

namespace {

const std::string multicall_method_name =
  "<member><name>methodName</name><value><string>xmlrpc_reflect</string></value></member>";
const std::string multicall_params =
  "<member><name>params</name><value><array><data>"
  "<value><string></string></value><value><string>a</string></value>"
  "</data></array></value></member>";

std::string
multicall_request(const std::string& members) {
  return "<?xml version=\"1.0\"?><methodCall><methodName>system.multicall</methodName>"
         "<params><param><value><array><data><value><struct>" + members +
         "</struct></value></data></array></value></param></params></methodCall>";
}

}

void
TestXmlrpc::test_multicall_bytes() {
  auto call = [this](const std::string& name, const std::string& value) {
    const auto input = multicall_request(
      "<member><name>methodName</name><value><string>" + name + "</string></value></member>"
      "<member><name>params</name><value><array><data><value><string></string></value>" +
      value + "</data></array></value></member>");
    std::string output;
    unsigned writes = 0;
    m_xmlrpc.process(input.c_str(), input.size(), [&output, &writes](const char* c, uint32_t l) {
      ++writes;
      output.append(c, l);
      return true;
    });
    CPPUNIT_ASSERT_EQUAL(1u, writes);
    return output;
  };
  const std::string binary = "<value><base64>AAENCv8=</base64></value>";
  const auto output = call("xmlrpc_reflect_string", binary);
  CPPUNIT_ASSERT_EQUAL(std::string("<?xml version=\"1.0\"?><methodResponse><params><param><value>"
    "<array><data><value><array><data>") + binary +
    "</data></array></value></data></array></value></param></params></methodResponse>", output);

  const auto fault = call("bad\x01method", "");
  CPPUNIT_ASSERT(fault.find("<name>faultString</name><value><string>escaped bytes: method 'bad\\x01method' not defined</string>") != std::string::npos);
  CPPUNIT_ASSERT(fault.find("<base64>") == std::string::npos);

  const auto key_fault = call("xmlrpc_map", binary);
  CPPUNIT_ASSERT_EQUAL(std::string("<?xml version=\"1.0\"?><methodResponse><fault><value><struct>"
    "<member><name>faultCode</name><value><i8>-501</i8></value></member>"
    "<member><name>faultString</name><value><string>struct member name is not XML 1.0 text</string></value></member>"
    "</struct></value></fault></methodResponse>"), key_fault);
}

void
TestXmlrpc::test_multicall_member_order() {
  auto call = [this](const std::string& input) {
    std::string output;
    m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
    return output;
  };

  // methodName first is the only order accepted; the positive control proves
  // the harness drives the real code path rather than a stub.
  std::string ordered = call(multicall_request(multicall_method_name + multicall_params));
  CPPUNIT_ASSERT(ordered.find("faultCode") == std::string::npos);

  // params before methodName is rejected with a clear top-level fault,
  // instead of the "could not find expected element string" of a positional read.
  std::string expected_fault =
    "<?xml version=\"1.0\"?><methodResponse><fault><value><struct>"
    "<member><name>faultCode</name><value><i8>-503</i8></value></member>"
    "<member><name>faultString</name><value><string>multicall struct's first member must be methodName</string></value></member>"
    "</struct></value></fault></methodResponse>";
  CPPUNIT_ASSERT_EQUAL(expected_fault, call(multicall_request(multicall_params + multicall_method_name)));
}

#else

void TestXmlrpc::test_invalid_utf8() {}
void TestXmlrpc::test_xml_string_bytes() {}
void TestXmlrpc::test_xml_fault_bytes() {}
void TestXmlrpc::test_xml_member_name() {}
void TestXmlrpc::test_basics() {}
void TestXmlrpc::test_size_limit() {}
void TestXmlrpc::test_response_size_limit() {}
void TestXmlrpc::test_multicall_member_order() {}
void TestXmlrpc::test_multicall_bytes() {}
void TestXmlrpc::setUp() {}
void TestXmlrpc::tearDown() {}

#endif
