#include "config.h"
#include <string>

#include "command_helpers.h"
#include "rpc/command_map.h"

#include "xmlrpc_test.h"
#include "control.h"
#include "globals.h"

CPPUNIT_TEST_SUITE_REGISTRATION(XmlrpcTest);

torrent::Object xmlrpc_cmd_test_reflect(rpc::target_type t, const torrent::Object& obj) { return obj; }

void initialize_command_dynamic();

#if defined(HAVE_XMLRPC_TINYXML2) && !defined(HAVE_XMLRPC_C)
void
XmlrpcTest::setUp() {
  m_commandItr = m_commands;
  m_xmlrpc = rpc::XmlRpc();
  m_xmlrpc.initialize();
  setlocale(LC_ALL, "");
  cachedTime = rak::timer::current();
  control = new Control;
  if (rpc::commands.find("xmlrpc_reflect") == rpc::commands.end()) {
    CMD2_ANY("xmlrpc_reflect", &xmlrpc_cmd_test_reflect);
  }
}

void
XmlrpcTest::test_basics() {
  std::ifstream file; file.open("rpc/xmlrpc_test_data.txt");
  CPPUNIT_ASSERT(file.good());
  std::vector<std::string> titles;
  std::vector<std::string> inputs;
  std::vector<std::string> outputs;
  std::string line;
  int index = 0;
  // Read file into inputs/outputs
  while (std::getline(file, line)) {
    if (line.size() == 0) {
      continue;
    }
    if (line[0] == '#') {
      titles.push_back(line);
      continue;
    }
    if (index % 2) {
      outputs.push_back(line);
    } else {
      inputs.push_back(line);
    }
    index++;
  }

  // Sanity check the above parser
  CPPUNIT_ASSERT_MESSAGE("Could not parse test data", inputs.size() > 0 && inputs.size() == outputs.size() && inputs.size() == titles.size());
  for (int i = 0; i < inputs.size(); i++) {
    std::string output;
    m_xmlrpc.process(inputs[i].c_str(), inputs[i].size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
    CPPUNIT_ASSERT_EQUAL_MESSAGE(titles[i], std::string(outputs[i]), output);
  }
}

void
XmlrpcTest::test_invalid_utf8() {
  // Surprisingly, this call doesn't fail. TinyXML-2 technically expects
  // valid UTF-8, but doesn't check strings, and Object strings are
  // just a series of bytes so it reflects just fine.
  std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><string>\xc3\x28</string></value></param></params></methodCall>";
  std::string expected = "<?xml version=\"1.0\"?><methodReponse><params><param><value><array><value><string>\xc3\x28</string></value></array></value></param></params></methodReponse>";
  std::string output;
  m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
  CPPUNIT_ASSERT_EQUAL(expected, output);
}

void
XmlrpcTest::test_size_limit() {
  std::string input = "<?xml version=\"1.0\"?><methodCall><methodName>xmlrpc_reflect</methodName><params><param><value><string></string></value></param><param><value><string>\xc3\x28</string></value></param></params></methodCall>";
  std::string expected = "<?xml version=\"1.0\"?><methodReponse><fault><struct><member><name>faultCode</name><value><i4>-509</i4></value></member><member><name>faultString</name><value><string>Content size exceeds maximum XML-RPC limit</string></value></member></struct></fault></methodReponse>";
  std::string output;
  m_xmlrpc.set_size_limit(1);
  m_xmlrpc.process(input.c_str(), input.size(), [&output](const char* c, uint32_t l){ output.append(c, l); return true;});
  CPPUNIT_ASSERT_EQUAL(expected, output);
}
#else
void XmlrpcTest::test_invalid_utf8() {}
void XmlrpcTest::test_basics() {}
void XmlrpcTest::test_size_limit() {}
void XmlrpcTest::setUp() {}
#endif
