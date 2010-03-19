#include "config.h"

#include <sstream>
#include <torrent/object.h>
#include "rpc/command_map.h"

#import "command_slot_test.h"

CPPUNIT_TEST_SUITE_REGISTRATION(CommandSlotTest);

torrent::Object cmd_test_a(rpc::target_type t, const torrent::Object& obj) { return obj; }
torrent::Object cmd_test_b(rpc::target_type t, const torrent::Object& obj, uint64_t c) { return torrent::Object(c); }

void
CommandSlotTest::test_basics() {
  rpc::command_base test_any;
  test_any.set_function<rpc::any_function>(&cmd_test_a);
  CPPUNIT_ASSERT(rpc::command_base_call_any(&test_any, rpc::make_target(), (int64_t)1).as_value() == 1);

  test_any.set_function<rpc::any_function>(std::tr1::bind(&cmd_test_b, std::tr1::placeholders::_1, std::tr1::placeholders::_2, (uint64_t)2));
  CPPUNIT_ASSERT(rpc::command_base_call_any(&test_any, rpc::make_target(), (int64_t)1).as_value() == 2);
}

void
CommandSlotTest::test_type_validity() {
  CPPUNIT_ASSERT((rpc::command_base_is_type<rpc::any_function, &rpc::command_base_call_any>::value));
  CPPUNIT_ASSERT((rpc::command_base_is_type<rpc::any_string_function, &rpc::command_base_call_any_string>::value));
}
