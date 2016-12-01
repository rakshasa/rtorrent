#include "config.h"

#include "test_parse_options.h"

#include "helpers/assert.h"

#include <array>
#include <torrent/connection_manager.h>
#include <torrent/exceptions.h>
#include <torrent/utils/log.h>
#include <torrent/utils/option_strings.h>

#include "rpc/parse_options.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestParseOptions);

static const int flag_1 = 1 << 0;
static const int flag_2 = 1 << 1;
static const int flag_3 = 1 << 2;
static const int flag_4 = 1 << 3;
static const int flag_5 = 1 << 4;
static const int flag_6 = 1 << 5;

static std::vector<std::pair<const char*, int>> flag_list = {
  { "foo", flag_1 },
  { "bar", flag_2 },
  { "baz", flag_3 },
  { "a12", flag_4 },
  { "not_bar", ~flag_2 },
  { "45b", flag_5 },
  { "a_b_c__e_3_g", flag_6 },
  { "not_a12", ~flag_4 },
};

static int
flag_to_int(const std::string& flag) {
  for (auto f : flag_list)
    if (f.first == flag)
      return f.second;
  
  throw torrent::input_error("unknown flag");
}

#define FLAG_ASSERT(flags, result)                                      \
  CPPUNIT_ASSERT(rpc::parse_option_flag(flags, std::bind(&flag_to_int, std::placeholders::_1)) == (result))

#define FLAGS_ASSERT(flags, result)                                     \
  CPPUNIT_ASSERT(rpc::parse_option_flags(flags, std::bind(&flag_to_int, std::placeholders::_1)) == (result))

#define FLAGS_ASSERT_VALUE(flags, value, result)                        \
  CPPUNIT_ASSERT(rpc::parse_option_flags(flags, std::bind(&flag_to_int, std::placeholders::_1), value) == (result))

#define FLAG_ASSERT_ERROR(flags)                                        \
  ASSERT_CATCH_INPUT_ERROR( { rpc::parse_option_flag(flags, std::bind(&flag_to_int, std::placeholders::_1)); } )

#define FLAGS_ASSERT_ERROR(flags)                                       \
  ASSERT_CATCH_INPUT_ERROR( { rpc::parse_option_flags(flags, std::bind(&flag_to_int, std::placeholders::_1)); } )

#define FLAGS_ASSERT_VALUE_ERROR(flags, value)                          \
  ASSERT_CATCH_INPUT_ERROR( { rpc::parse_option_flags(flags, std::bind(&flag_to_int, std::placeholders::_1), value); } )

void
TestParseOptions::test_flag_basic() {
  FLAG_ASSERT("foo", flag_1);

  FLAG_ASSERT(" foo ", flag_1);
  FLAG_ASSERT("    foo     ", flag_1);

  FLAG_ASSERT("a12", flag_4);
  FLAG_ASSERT("45b", flag_5);
  FLAG_ASSERT("a_b_c__e_3_g", flag_6);
}

void
TestParseOptions::test_flag_error() {
  FLAG_ASSERT_ERROR("");
  FLAG_ASSERT_ERROR("foo|bar");
  FLAG_ASSERT_ERROR("foo|bar|baz");
 
  FLAG_ASSERT_ERROR("foo |bar");
  FLAG_ASSERT_ERROR("foo | bar| baz");
}

void
TestParseOptions::test_flags_basic() {
  FLAGS_ASSERT("", 0);
  FLAGS_ASSERT("foo", flag_1);
  FLAGS_ASSERT("foo|bar", flag_1 | flag_2);
  FLAGS_ASSERT("foo|bar|baz", flag_1 | flag_2 | flag_3);

  FLAGS_ASSERT(" foo ", flag_1);
  FLAGS_ASSERT("    foo     ", flag_1);
  FLAGS_ASSERT("foo |bar", flag_1 | flag_2);
  FLAGS_ASSERT("foo | bar| baz", flag_1 | flag_2 | flag_3);

  FLAGS_ASSERT("a12", flag_4);
  FLAGS_ASSERT("45b", flag_5);
  FLAGS_ASSERT("a_b_c__e_3_g", flag_6);
}

void
TestParseOptions::test_flags_error() {
  FLAGS_ASSERT_ERROR("fooa");
  FLAGS_ASSERT_ERROR("afoo");

  FLAGS_ASSERT_ERROR("|");
  FLAGS_ASSERT_ERROR("foo|");
  FLAGS_ASSERT_ERROR("|foo");
  FLAGS_ASSERT_ERROR(",foo");
}

void
TestParseOptions::test_flags_complex() {
  FLAGS_ASSERT_VALUE("", 0, 0);
  FLAGS_ASSERT_VALUE("", flag_1, flag_1);
  FLAGS_ASSERT_VALUE("bar", flag_1, flag_1 | flag_2);
  FLAGS_ASSERT_VALUE("bar|baz", flag_1, flag_1 | flag_2 | flag_3);

  FLAGS_ASSERT_VALUE("not_bar", flag_2, 0);
  FLAGS_ASSERT_VALUE("not_bar|not_a12", flag_2, 0);

  FLAGS_ASSERT_VALUE("bar|not_a12", flag_3 | flag_4, flag_2 | flag_3);
}

#define FLAG_LT_LOG_ASSERT(flags, result)                               \
  CPPUNIT_ASSERT(rpc::parse_option_flag(flags, std::bind(&torrent::option_find_string_str, torrent::OPTION_LOG_GROUP, std::placeholders::_1)) == (result))

#define FLAG_LT_LOG_ASSERT_ERROR(flags)                                 \
  ASSERT_CATCH_INPUT_ERROR(rpc::parse_option_flag(flags, std::bind(&torrent::option_find_string_str, torrent::OPTION_LOG_GROUP, std::placeholders::_1)))

void
TestParseOptions::test_flag_libtorrent() {
  FLAG_LT_LOG_ASSERT("resume_data", torrent::LOG_RESUME_DATA);

  FLAG_LT_LOG_ASSERT_ERROR("resume_data|rpc_dump");
}

#define FLAGS_LT_ENCRYPTION_ASSERT(flags, result)                       \
  CPPUNIT_ASSERT(rpc::parse_option_flags(flags, std::bind(&torrent::option_find_string_str, torrent::OPTION_ENCRYPTION, std::placeholders::_1)) == (result))

#define FLAGS_LT_ENCRYPTION_ASSERT_ERROR(flags)                         \
  ASSERT_CATCH_INPUT_ERROR(rpc::parse_option_flags(flags, std::bind(&torrent::option_find_string_str, torrent::OPTION_ENCRYPTION, std::placeholders::_1)))

void
TestParseOptions::test_flags_libtorrent() {
  FLAGS_LT_ENCRYPTION_ASSERT("", torrent::ConnectionManager::encryption_none);
  FLAGS_LT_ENCRYPTION_ASSERT("none", torrent::ConnectionManager::encryption_none);
  FLAGS_LT_ENCRYPTION_ASSERT("require_rc4", torrent::ConnectionManager::encryption_require_RC4);
  FLAGS_LT_ENCRYPTION_ASSERT("require_RC4", torrent::ConnectionManager::encryption_require_RC4);
  FLAGS_LT_ENCRYPTION_ASSERT("require_RC4 | enable_retry", torrent::ConnectionManager::encryption_require_RC4 | torrent::ConnectionManager::encryption_enable_retry);

  FLAGS_LT_ENCRYPTION_ASSERT_ERROR("require_");
}
