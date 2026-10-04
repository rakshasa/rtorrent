#include "config.h"

#include "test/src/test_command_groups.h"

#include <torrent/torrent.h>

#include "control.h"
#include "globals.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandGroups);

void initialize_command_groups();

static void
call_set(const char* value) {
  torrent::Object::list_type args;
  args.push_back(torrent::Object(int64_t{0}));
  args.push_back(torrent::Object(std::string(value)));

  rpc::commands.call_command("choke_group.up.max.set", torrent::Object::create_list_range(args.begin(), args.end()));
}

static int64_t
call_get(const char* key) {
  return rpc::commands.call_command(key, torrent::Object(int64_t{0})).as_value();
}

void
TestCommandGroups::setUp() {
  torrent::initialize_main_thread();
  torrent::initialize();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has("choke_group.up.max.set"))
    initialize_command_groups();
}

void
TestCommandGroups::tearDown() {
  torrent::cleanup();
}

void
TestCommandGroups::test_max_unchoked_in_range() {
  call_set("50");
  CPPUNIT_ASSERT_EQUAL(int64_t{50}, call_get("choke_group.up.max"));
  CPPUNIT_ASSERT_EQUAL(int64_t{0}, call_get("choke_group.up.max.unlimited"));

  call_set("-1");
  CPPUNIT_ASSERT_EQUAL(int64_t{1}, call_get("choke_group.up.max.unlimited"));
}

void
TestCommandGroups::test_max_unchoked_out_of_range() {
  call_set("50");

  CPPUNIT_ASSERT_THROW(call_set("4294967296"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(int64_t{50}, call_get("choke_group.up.max"));

  CPPUNIT_ASSERT_THROW(call_set("-2"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(int64_t{50}, call_get("choke_group.up.max"));
}
