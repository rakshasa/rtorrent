#include "config.h"

#include "command_helpers.h"
#include "rpc/command_map.h"

#include "command_map_test.h"

CPPUNIT_TEST_SUITE_REGISTRATION(CommandMapTest);

#undef CMD2_A_FUNCTION

#define CMD2_A_FUNCTION(key, function, slot, parm, doc)      \
  m_map.insert_slot<rpc::command_base_is_type<rpc::function>::type>(key, slot, &rpc::function,   \
                    rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, NULL, NULL);

torrent::Object cmd_test_map_a(rpc::target_type t, const torrent::Object& obj) { return obj; }
torrent::Object cmd_test_map_b(rpc::target_type t, const torrent::Object& obj, uint64_t c) { return torrent::Object(c); }

torrent::Object cmd_test_any_string(__UNUSED rpc::target_type target, const std::string& rawArgs) { return (int64_t)3; }

void
CommandMapTest::test_basics() {
  CMD2_ANY("test_a", &cmd_test_map_a);
  CMD2_ANY("test_b", std::bind(&cmd_test_map_b, std::placeholders::_1, std::placeholders::_2, (uint64_t)2));
  CMD2_ANY_STRING("any_string", &cmd_test_any_string);

  CPPUNIT_ASSERT(m_map.call_command("test_a", (int64_t)1).as_value() == 1);
  CPPUNIT_ASSERT(m_map.call_command("test_b", (int64_t)1).as_value() == 2);
  CPPUNIT_ASSERT(m_map.call_command("any_string", "").as_value() == 3);
}
