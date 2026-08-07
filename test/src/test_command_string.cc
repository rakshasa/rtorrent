#include "config.h"

#include "test/src/test_command_string.h"

#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandString);

void initialize_command_string();

namespace {

torrent::Object
args(std::initializer_list<torrent::Object> objects) {
  auto result = torrent::Object::create_list();

  for (const auto& object : objects)
    result.as_list().push_back(object);

  return result;
}

std::string
call_string(const char* key, std::initializer_list<torrent::Object> objects) {
  return rpc::commands.call_command(key, args(objects)).as_string();
}

int64_t
call_value(const char* key, std::initializer_list<torrent::Object> objects) {
  return rpc::commands.call_command(key, args(objects)).as_value();
}

torrent::Object::list_type
call_list(const char* key, std::initializer_list<torrent::Object> objects) {
  return rpc::commands.call_command(key, args(objects)).as_list();
}

// Runs a command the way a line in the configuration file would.
torrent::Object
parse(const char* command) {
  return rpc::parse_command_single(rpc::make_target(), command);
}

}

void
TestCommandString::setUp() {
  if (!rpc::commands.has("string.length"))
    initialize_command_string();
}

void
TestCommandString::tearDown() {
}

void
TestCommandString::test_length() {
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.length", {""}));
  CPPUNIT_ASSERT_EQUAL(int64_t(3), call_value("string.length", {"abc"}));

  // The length is counted in utf-8 characters, not bytes.
  CPPUNIT_ASSERT_EQUAL(int64_t(5), call_value("string.length", {"héllo"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(3), call_value("string.length", {"日本語"}));

  // Values are converted to their string representation.
  CPPUNIT_ASSERT_EQUAL(int64_t(4), call_value("string.length", {int64_t(1234)}));
}

void
TestCommandString::test_equals() {
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.equals", {"abc", "abc"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.equals", {"abc", "abd"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.equals", {"abc", "ab"}));

  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.equals", {"abc", "x", "abc"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.equals", {"abc", "x", "y"}));

  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.equals", {int64_t(42), "42"}));
}

void
TestCommandString::test_starts_with() {
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.starts_with", {"abcdef", "abc"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.starts_with", {"abcdef", ""}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.starts_with", {"abcdef", "bcd"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.starts_with", {"ab", "abc"}));

  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.starts_with", {"abcdef", "x", "ab"}));
}

void
TestCommandString::test_ends_with() {
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.ends_with", {"abcdef", "def"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.ends_with", {"abcdef", ""}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.ends_with", {"abcdef", "cde"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.ends_with", {"ef", "def"}));

  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.ends_with", {"a.torrent", ".rar", ".torrent"}));
}

void
TestCommandString::test_contains() {
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.contains", {"abcdef", "cde"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.contains", {"abcdef", "ace"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.contains", {"abcdef", "x", "bcd"}));

  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.contains", {"retracker.local", "RETRACKER"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.contains_i", {"retracker.local", "RETRACKER"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(1), call_value("string.contains_i", {"RETRACKER.LOCAL", "retracker"}));
  CPPUNIT_ASSERT_EQUAL(int64_t(0), call_value("string.contains_i", {"abcdef", "xyz"}));
}

void
TestCommandString::test_substr() {
  CPPUNIT_ASSERT_EQUAL(std::string("abcdef"), call_string("string.substr", {"abcdef"}));
  CPPUNIT_ASSERT_EQUAL(std::string("cdef"), call_string("string.substr", {"abcdef", int64_t(2)}));
  CPPUNIT_ASSERT_EQUAL(std::string("cde"), call_string("string.substr", {"abcdef", int64_t(2), int64_t(3)}));
  CPPUNIT_ASSERT_EQUAL(std::string("cdef"), call_string("string.substr", {"abcdef", int64_t(2), int64_t(100)}));
  CPPUNIT_ASSERT_EQUAL(std::string(""), call_string("string.substr", {"abcdef", int64_t(2), int64_t(0)}));

  // Negative positions are relative to the end of the string.
  CPPUNIT_ASSERT_EQUAL(std::string("ef"), call_string("string.substr", {"abcdef", int64_t(-2)}));
  CPPUNIT_ASSERT_EQUAL(std::string("e"), call_string("string.substr", {"abcdef", int64_t(-2), int64_t(1)}));

  // Out-of-bounds positions return the default value.
  CPPUNIT_ASSERT_EQUAL(std::string(""), call_string("string.substr", {"abcdef", int64_t(10)}));
  CPPUNIT_ASSERT_EQUAL(std::string("n/a"), call_string("string.substr", {"abcdef", int64_t(10), int64_t(1), "n/a"}));
  CPPUNIT_ASSERT_EQUAL(std::string("n/a"), call_string("string.substr", {"abcdef", int64_t(-10), int64_t(1), "n/a"}));

  // Positions and counts are in utf-8 characters, not bytes.
  CPPUNIT_ASSERT_EQUAL(std::string("本"), call_string("string.substr", {"日本語", int64_t(1), int64_t(1)}));
  CPPUNIT_ASSERT_EQUAL(std::string("本語"), call_string("string.substr", {"日本語", int64_t(-2)}));
}

void
TestCommandString::test_split() {
  auto parts = call_list("string.split", {"a,b,c", ","});

  CPPUNIT_ASSERT_EQUAL(size_t(3), parts.size());
  CPPUNIT_ASSERT_EQUAL(std::string("a"), parts.front().as_string());
  CPPUNIT_ASSERT_EQUAL(std::string("c"), parts.back().as_string());

  // Empty fields are preserved.
  CPPUNIT_ASSERT_EQUAL(size_t(3), call_list("string.split", {"a,,b", ","}).size());
  CPPUNIT_ASSERT_EQUAL(size_t(1), call_list("string.split", {"abc", ","}).size());

  // A multi-character delimiter is matched as a whole.
  CPPUNIT_ASSERT_EQUAL(size_t(2), call_list("string.split", {"a::b", "::"}).size());

  // An empty delimiter splits the text into its utf-8 characters.
  auto characters = call_list("string.split", {"日本語", ""});

  CPPUNIT_ASSERT_EQUAL(size_t(3), characters.size());
  CPPUNIT_ASSERT_EQUAL(std::string("日"), characters.front().as_string());
}

void
TestCommandString::test_join() {
  CPPUNIT_ASSERT_EQUAL(std::string("a, b"), call_string("string.join", {", ", "a", "b"}));
  CPPUNIT_ASSERT_EQUAL(std::string("ab"), call_string("string.join", {"", "a", "b"}));
  CPPUNIT_ASSERT_EQUAL(std::string(""), call_string("string.join", {", "}));
  CPPUNIT_ASSERT_EQUAL(std::string("a"), call_string("string.join", {", ", "a"}));
  CPPUNIT_ASSERT_EQUAL(std::string("1-2"), call_string("string.join", {"-", int64_t(1), int64_t(2)}));

  // Lists are flattened, so the output of string.split can be joined again.
  CPPUNIT_ASSERT_EQUAL(std::string("a-b-c"), call_string("string.join", {"-", args({"a", "b"}), "c"}));
}

void
TestCommandString::test_pad() {
  CPPUNIT_ASSERT_EQUAL(std::string("  7"), call_string("string.lpad", {"7", int64_t(3)}));
  CPPUNIT_ASSERT_EQUAL(std::string("7  "), call_string("string.rpad", {"7", int64_t(3)}));
  CPPUNIT_ASSERT_EQUAL(std::string("007"), call_string("string.lpad", {"7", int64_t(3), "0"}));
  CPPUNIT_ASSERT_EQUAL(std::string("700"), call_string("string.rpad", {"7", int64_t(3), "0"}));

  // Text that is already long enough is returned unchanged.
  CPPUNIT_ASSERT_EQUAL(std::string("abcd"), call_string("string.lpad", {"abcd", int64_t(2)}));
  CPPUNIT_ASSERT_EQUAL(std::string("abcd"), call_string("string.rpad", {"abcd", int64_t(4)}));

  // Multi-character padding is repeated, and an empty padding is a no-op.
  CPPUNIT_ASSERT_EQUAL(std::string("axyx"), call_string("string.rpad", {"a", int64_t(4), "xy"}));
  CPPUNIT_ASSERT_EQUAL(std::string("a"), call_string("string.rpad", {"a", int64_t(4), ""}));

  // Padding is counted in utf-8 characters, not bytes.
  CPPUNIT_ASSERT_EQUAL(std::string("00日"), call_string("string.lpad", {"日", int64_t(3), "0"}));
}

void
TestCommandString::test_strip() {
  CPPUNIT_ASSERT_EQUAL(std::string("a b"), call_string("string.strip", {"  a b  "}));
  CPPUNIT_ASSERT_EQUAL(std::string("a"), call_string("string.strip", {"\t\n a \r\n"}));
  CPPUNIT_ASSERT_EQUAL(std::string("a  "), call_string("string.lstrip", {"  a  "}));
  CPPUNIT_ASSERT_EQUAL(std::string("  a"), call_string("string.rstrip", {"  a  "}));

  CPPUNIT_ASSERT_EQUAL(std::string("a"), call_string("string.strip", {"xxaxx", "x"}));
  CPPUNIT_ASSERT_EQUAL(std::string("a"), call_string("string.strip", {"/x/a/x/", "/", "x"}));
  CPPUNIT_ASSERT_EQUAL(std::string(""), call_string("string.strip", {"aaa", "a"}));
  CPPUNIT_ASSERT_EQUAL(std::string("  a  "), call_string("string.strip", {"  a  ", ""}));

  // The strippable argument is a set of utf-8 characters.
  CPPUNIT_ASSERT_EQUAL(std::string("a"), call_string("string.strip", {"日a日", "日"}));
}

void
TestCommandString::test_map() {
  CPPUNIT_ASSERT_EQUAL(std::string("b"), call_string("string.map", {"a", args({"a", "b"})}));
  CPPUNIT_ASSERT_EQUAL(std::string("c"), call_string("string.map", {"c", args({"a", "b"})}));

  // Only whole-string matches are replaced.
  CPPUNIT_ASSERT_EQUAL(std::string("ab"), call_string("string.map", {"ab", args({"a", "b"})}));

  // The first matching pair wins.
  CPPUNIT_ASSERT_EQUAL(std::string("y"), call_string("string.map", {"x", args({"a", "b"}), args({"x", "y"}), args({"x", "z"})}));
}

void
TestCommandString::test_replace() {
  CPPUNIT_ASSERT_EQUAL(std::string("a+b+c"), call_string("string.replace", {"a-b-c", args({"-", "+"})}));
  CPPUNIT_ASSERT_EQUAL(std::string("abc"), call_string("string.replace", {"a-b-c", args({"-", ""})}));
  CPPUNIT_ASSERT_EQUAL(std::string("a-b-c"), call_string("string.replace", {"a-b-c", args({"x", "y"})}));

  // Pairs are applied in order, left to right.
  CPPUNIT_ASSERT_EQUAL(std::string("xby"), call_string("string.replace", {"abc", args({"a", "x"}), args({"c", "y"})}));

  // A replacement that contains the replaced text terminates.
  CPPUNIT_ASSERT_EQUAL(std::string("aaaa"), call_string("string.replace", {"aa", args({"a", "aa"})}));
}

void
TestCommandString::test_invalid_arguments() {
  CPPUNIT_ASSERT_THROW(rpc::commands.call_command("string.length", torrent::Object()), torrent::input_error);
  CPPUNIT_ASSERT_THROW(call_string("string.length", {"a", "b"}), torrent::input_error);
  CPPUNIT_ASSERT_THROW(call_value("string.equals", {"a"}), torrent::input_error);
  CPPUNIT_ASSERT_THROW(call_value("string.contains", {"a"}), torrent::input_error);
  CPPUNIT_ASSERT_THROW(call_list("string.split", {"a"}), torrent::input_error);
  CPPUNIT_ASSERT_THROW(call_string("string.lpad", {"a"}), torrent::input_error);

  // The character count of string.substr cannot be negative.
  CPPUNIT_ASSERT_THROW(call_string("string.substr", {"abc", int64_t(0), int64_t(-1)}), torrent::input_error);

  // Both string.map and string.replace require {old, new} pairs.
  CPPUNIT_ASSERT_THROW(call_string("string.map", {"a", "b"}), torrent::input_error);
  CPPUNIT_ASSERT_THROW(call_string("string.replace", {"a", args({"a", "b", "c"})}), torrent::input_error);
  CPPUNIT_ASSERT_THROW(call_string("string.replace", {"a", args({"", "b"})}), torrent::input_error);
}

void
TestCommandString::test_config_syntax() {
  CPPUNIT_ASSERT_EQUAL(int64_t(3), parse("string.length=abc").as_value());
  CPPUNIT_ASSERT_EQUAL(int64_t(1), parse("string.contains=retracker.local,retracker").as_value());
  CPPUNIT_ASSERT_EQUAL(int64_t(1), parse("string.starts_with=udp://tracker.example.com,http://,udp://").as_value());
  CPPUNIT_ASSERT_EQUAL(std::string("cde"), parse("string.substr=abcdef,2,3").as_string());
  CPPUNIT_ASSERT_EQUAL(std::string("padded"), parse("string.strip=\"  padded  \"").as_string());

  // The {old, new} pairs are written as a block in the configuration file.
  CPPUNIT_ASSERT_EQUAL(std::string("a+b+c"), parse("string.replace=a-b-c,{-,+}").as_string());
  CPPUNIT_ASSERT_EQUAL(std::string("y"), parse("string.map=x,{a,b},{x,y}").as_string());

  // Commands nest, so a split can be joined back together.
  CPPUNIT_ASSERT_EQUAL(std::string("a-b-c"), parse("string.join=-,(string.split,a.b.c,.)").as_string());
}
