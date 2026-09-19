#include "config.h"

#include "test/rpc/test_parse.h"

#include <cstdint>
#include <limits>

#include "rpc/parse.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestParse);

void
TestParse::test_whole_value_in_range() {
  int64_t value = 0;

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("9223372036854775807", &value));
  CPPUNIT_ASSERT_EQUAL(std::numeric_limits<int64_t>::max(), value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("-9223372036854775808", &value));
  CPPUNIT_ASSERT_EQUAL(std::numeric_limits<int64_t>::min(), value);
}

void
TestParse::test_whole_value_out_of_range() {
  int64_t value = 0;

  CPPUNIT_ASSERT(!rpc::parse_whole_value_nothrow("9223372036854775808", &value));
  CPPUNIT_ASSERT(!rpc::parse_whole_value_nothrow("-9223372036854775809", &value));
  CPPUNIT_ASSERT(!rpc::parse_whole_value_nothrow("99999999999999999999999999", &value));
}

void
TestParse::test_whole_value_bases() {
  int64_t value = 0;

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("0x1f", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{31}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("0X1F", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{31}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("-0x1f", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{-31}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("0022", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{18}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("22", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{22}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("1f", &value, 16));
  CPPUNIT_ASSERT_EQUAL(int64_t{31}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("0x1f", &value, 16));
  CPPUNIT_ASSERT_EQUAL(int64_t{31}, value);
}

void
TestParse::test_whole_value_prefixes() {
  int64_t value = 0;

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("+5", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{5}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("  5  ", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{5}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("5k", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{5120}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("yes", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{1}, value);

  CPPUNIT_ASSERT(rpc::parse_whole_value_nothrow("false", &value));
  CPPUNIT_ASSERT_EQUAL(int64_t{0}, value);

  CPPUNIT_ASSERT(!rpc::parse_whole_value_nothrow("junk", &value));
  CPPUNIT_ASSERT(!rpc::parse_whole_value_nothrow("", &value));
}
