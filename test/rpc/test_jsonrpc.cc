#include "config.h"

#include "test/rpc/test_jsonrpc.h"

#include <string>

#include "control.h"
#include "globals.h"
#include "command_helpers.h"
#include "rpc/command_map.h"
#include "rpc/scgi_task.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestJsonrpc);

torrent::Object
jsonrpc_cmd_test_reflect([[maybe_unused]] rpc::target_type t, const torrent::Object& obj) { return obj; }

torrent::Object
jsonrpc_cmd_test_oversized([[maybe_unused]] rpc::target_type t, [[maybe_unused]] const torrent::Object& obj) {
  return torrent::Object(std::string(rpc::SCgiTask::max_response_size + (1 << 20), 'a'));
}

void initialize_command_dynamic();

// Name, Request, Expected response
std::vector<std::tuple<std::string, std::string, std::string>> basic_jsonrpc_requests = {
  std::make_tuple("Basic call",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[]})"),

  std::make_tuple("Basic call with empty params",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[]})"),

  std::make_tuple("Basic call without params",
                  R"({"jsonrpc": "2.0",	"method": "jsonrpc_reflect", "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[]})"),

  std::make_tuple("Basic call with null id",
                  R"({"jsonrpc": "2.0",	"method": "jsonrpc_reflect", "id": null})",
                  R"({"id":null,"jsonrpc":"2.0","result":[]})"),

  std::make_tuple("Basic call with string id",
                  R"({"jsonrpc": "2.0",	"method": "jsonrpc_reflect", "id": "1"})",
                  R"({"id":"1","jsonrpc":"2.0","result":[]})"),

  std::make_tuple("UTF-8 string",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", "чао"], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":["чао"]})"),

  std::make_tuple("Emoji string",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", "😊"], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":["😊"]})"),

  std::make_tuple("Small int",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", 41], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[41]})"),

  std::make_tuple("Large int",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", 2247483647], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[2247483647]})"),

  std::make_tuple("Boolean",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", true], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[1]})"),

  std::make_tuple("Negative large ints",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", -2247483647], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[-2247483647]})"),

  std::make_tuple("Simple array",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", [2247483647]], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[[2247483647]]})"),

  std::make_tuple("Empty array",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", []], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[[]]})"),

  std::make_tuple("Empty struct",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", {}], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[{}]})"),

  std::make_tuple("Simple struct",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", {"lowerBound": 18}], "id": 1})",
                  R"({"id":1,"jsonrpc":"2.0","result":[{"lowerBound":18}]})"),

  std::make_tuple("Notification",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""]})",
                  ""),

  std::make_tuple("Batch",
                  R"([{"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""], "id": 1},{"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""], "id": 2}])",
                  R"([{"id":1,"jsonrpc":"2.0","result":[]},{"id":2,"jsonrpc":"2.0","result":[]}])"),

  std::make_tuple("Batch with notification",
                  R"([{"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""], "id": 1},{"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""]}])",
                  R"([{"id":1,"jsonrpc":"2.0","result":[]}])"),

  std::make_tuple("Invalid - empty batch",
                  "[]",
                  R"({"error":{"code":-32600,"message":"invalid request: empty batch"},"id":null,"jsonrpc":"2.0"})"),

  std::make_tuple("Invalid - missing method",
                  R"({"jsonrpc": "2.0", "method": "no_such_method", "id": 1})",
                  R"({"error":{"code":-32601,"message":"method not found: no_such_method"},"id":1,"jsonrpc":"2.0"})"),

  std::make_tuple("Invalid - i8 target",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [41], "id": 1})",
                  R"({"error":{"code":-32602,"message":"invalid parameters: target must be a string"},"id":1,"jsonrpc":"2.0"})"),

  std::make_tuple("Invalid - broken JSON",
                  R"(nrpc": "2.0", "method": "jsonrpc_reflect", "params": [41], ")",
                  R"({"error":{"code":-32700,"message":"[json.exception.parse_error.101] parse error at line 1, column 2: syntax error while parsing value - invalid literal; last read: 'nr'"},"id":null,"jsonrpc":"2.0"})"),

  std::make_tuple("Invalid - float",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", 3.14], "id": 1})",
                  R"({"error":{"code":-32602,"message":"invalid parameters: unexpected data type float"},"id":1,"jsonrpc":"2.0"})"),

  std::make_tuple("Invalid - invalid UTF-8",
                  R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", ")"
                  "\xc3\x28"
                  R"("], "id": 1})",
                  R"({"error":{"code":-32700,"message":"[json.exception.parse_error.101] parse error at line 1, column 66: syntax error while parsing value - invalid string: ill-formed UTF-8 byte; last read: '\"�('"},"id":null,"jsonrpc":"2.0"})"),
};

void
TestJsonrpc::setUp() {
  m_test_main_thread = TestMainThread::create();
  m_test_main_thread->init_thread();

  m_jsonrpc = rpc::JsonRpc();
  m_jsonrpc.initialize();
  setlocale(LC_ALL, "");
  control = new Control;

  if (rpc::commands.find("jsonrpc_reflect") == rpc::commands.end()) {
    CMD2_ANY("jsonrpc_reflect", &jsonrpc_cmd_test_reflect);
  }

  if (rpc::commands.find("jsonrpc_oversized") == rpc::commands.end()) {
    CMD2_ANY("jsonrpc_oversized", &jsonrpc_cmd_test_oversized);
  }
}

void
TestJsonrpc::tearDown() {
  m_test_main_thread.reset();
}

void
TestJsonrpc::test_basics() {
  for (auto& test : basic_jsonrpc_requests) {
    std::string output;
    m_jsonrpc.process(std::get<1>(test).c_str(), std::get<1>(test).size(), [&output](const char* c, uint32_t l) { output.append(c, l); return true; });
    CPPUNIT_ASSERT_EQUAL_MESSAGE(std::get<0>(test), std::get<2>(test), output);
  }
}

// A command whose result does not fit in the SCGI response buffer must be
// answered with a fault, not handed to the writer. SCgiTask::receive_write
// treats an oversized body as an internal_error, which is not caught by any
// RPC handler and terminates the process.
void
TestJsonrpc::test_response_size_limit() {
  const std::string request = R"({"jsonrpc": "2.0", "method": "jsonrpc_oversized", "params": [""], "id": 1})";
  const std::string expected = R"({"error":{"code":-32000,"message":"response size exceeds maximum RPC limit"},"id":1,"jsonrpc":"2.0"})";

  std::string output;
  m_jsonrpc.process(request.c_str(), request.size(), [&output](const char* c, uint32_t l) { output.append(c, l); return true; });

  CPPUNIT_ASSERT_MESSAGE("response handed to the writer is " + std::to_string(output.size()) +
                         " bytes, over the " + std::to_string(rpc::SCgiTask::max_response_size) + " byte SCGI limit",
                         output.size() <= rpc::SCgiTask::max_response_size);
  CPPUNIT_ASSERT_EQUAL(expected, output);
}

// Nesting has to be rejected by scanning the input, not by parsing it:
// nlohmann materialises the whole tree first, costing tens of bytes of heap
// per input byte, and json_to_object only ever walks "params".
void
TestJsonrpc::test_depth_limit() {
  // A JSON string holding a quote and brackets that must not be counted.
  const std::string tricky = R"("\"[[[")";

  std::vector<std::tuple<std::string, std::string, std::string>> requests = {
    std::make_tuple("Nesting under the limit is accepted",
                    R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", )" +
                      std::string(1000, '[') + std::string(1000, ']') + R"(], "id": 1})",
                    R"({"id":1,"jsonrpc":"2.0","result":[)" +
                      std::string(1000, '[') + std::string(1000, ']') + R"(]})"),

    // Nesting outside "params" is never converted, so only a scan of the
    // input can reject it before the parser has paid for it.
    std::make_tuple("Nesting outside params is rejected",
                    R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""], "id": 1, "x": )" +
                      std::string(2000, '[') + std::string(2000, ']') + R"(})",
                    R"({"error":{"code":-32600,"message":"maximum nesting depth exceeded"},"id":null,"jsonrpc":"2.0"})"),

    std::make_tuple("Nesting over the limit is rejected",
                    R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", )" +
                      std::string(2000, '[') + std::string(2000, ']') + R"(], "id": 1})",
                    R"({"error":{"code":-32600,"message":"maximum nesting depth exceeded"},"id":null,"jsonrpc":"2.0"})"),

    std::make_tuple("Brackets inside a string are not nesting",
                    R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", ")" +
                      std::string(2000, '[') + R"("], "id": 1})",
                    R"({"id":1,"jsonrpc":"2.0","result":[")" +
                      std::string(2000, '[') + R"("]})"),

    std::make_tuple("An escaped quote does not end a string",
                    R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": ["", )" + tricky + R"(, ")" +
                      std::string(2000, '[') + R"("], "id": 1})",
                    R"({"id":1,"jsonrpc":"2.0","result":[)" + tricky + R"(,")" +
                      std::string(2000, '[') + R"("]})"),
  };

  for (auto& test : requests) {
    std::string output;
    m_jsonrpc.process(std::get<1>(test).c_str(), std::get<1>(test).size(), [&output](const char* c, uint32_t l) { output.append(c, l); return true; });
    CPPUNIT_ASSERT_EQUAL_MESSAGE(std::get<0>(test), std::get<2>(test), output);
  }
}

// network.xmlrpc.size_limit is the only knob bounding how much input a single
// request may spend memory on, and it has to bound the JSON path too.
void
TestJsonrpc::test_size_limit() {
  const std::string request = R"({"jsonrpc": "2.0", "method": "jsonrpc_reflect", "params": [""], "id": 1})";
  const std::string expected = R"({"error":{"code":-32600,"message":"content size exceeds maximum RPC limit"},"id":null,"jsonrpc":"2.0"})";

  std::string output;
  m_jsonrpc.set_size_limit(1);
  m_jsonrpc.process(request.c_str(), request.size(), [&output](const char* c, uint32_t l) { output.append(c, l); return true; });

  CPPUNIT_ASSERT_EQUAL(expected, output);
}
