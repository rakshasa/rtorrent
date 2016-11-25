#include "config.h"

#include "test_parse_options.h"

#include "helpers/assert.h"

#include <array>
#include <torrent/exceptions.h>

#include "rpc/parse_options.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestParseOptions);

static const int flag_foo = 1 << 0;
static const int flag_bar = 1 << 1;
static const int flag_baz = 1 << 2;
static const int flag_a12 = 1 << 3;
static const int flag_45b = 1 << 4;
static const int flag_6   = 1 << 5;

static std::vector<std::pair<const char*, int>> flag_list = {
  { "foo", flag_foo },
  { "bar", flag_bar },
  { "baz", flag_baz },
  { "a12", flag_a12 },
  { "45b", flag_45b },
  { "a_b_c__e_3_g", flag_6 },
};

static int
flag_to_int(const std::string& flag) {
  for (auto f : flag_list)
    if (f.first == flag)
      return f.second;
  
  throw torrent::input_error("unknown flag");
}

#define FLAGS_ASSERT(flags, result)                                     \
  CPPUNIT_ASSERT(rpc::parse_option_flags(flags, std::bind(&flag_to_int, std::placeholders::_1)) == (result))

#define FLAGS_ASSERT_ERROR(flags)                                       \
  ASSERT_CATCH_INPUT_ERROR( { rpc::parse_option_flags(flags, std::bind(&flag_to_int, std::placeholders::_1)); } )

void
TestParseOptions::test_basic() {
  FLAGS_ASSERT("", 0);
  FLAGS_ASSERT("foo", flag_foo);
  FLAGS_ASSERT("foo|bar", flag_foo | flag_bar);
  FLAGS_ASSERT("foo|bar|baz", flag_foo | flag_bar | flag_baz);

  FLAGS_ASSERT(" foo ", flag_foo);
  FLAGS_ASSERT("    foo     ", flag_foo);
  FLAGS_ASSERT("foo |bar", flag_foo | flag_bar);
  FLAGS_ASSERT("foo | bar| baz", flag_foo | flag_bar | flag_baz);

  FLAGS_ASSERT("a12", flag_a12);
  FLAGS_ASSERT("45b", flag_45b);
  FLAGS_ASSERT("a_b_c__e_3_g", flag_6);
}

void
TestParseOptions::test_errors() {
  FLAGS_ASSERT_ERROR("fooa");
  FLAGS_ASSERT_ERROR("afoo");

  FLAGS_ASSERT_ERROR("|");
  FLAGS_ASSERT_ERROR("foo|");
  FLAGS_ASSERT_ERROR("|foo");
  FLAGS_ASSERT_ERROR(",foo");
}
