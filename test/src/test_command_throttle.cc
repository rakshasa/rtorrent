#include "config.h"

#include "test/src/test_command_throttle.h"

#include <torrent/throttle.h>
#include <torrent/torrent.h>

#include "core/manager.h"
#include "control.h"
#include "globals.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandThrottle);

void initialize_command_throttle();

static void
call_set(const char* key, const char* value) {
  rpc::commands.call_command(key, torrent::Object(std::string(value)));
}

static void
call_named(const char* key, const char* name, const char* value) {
  torrent::Object::list_type args;
  args.push_back(torrent::Object(std::string(name)));
  args.push_back(torrent::Object(std::string(value)));

  rpc::commands.call_command(key, torrent::Object::create_list_range(args.begin(), args.end()));
}

static uint64_t
down_rate() {
  return torrent::down_throttle_global()->max_rate();
}

void
TestCommandThrottle::setUp() {
  torrent::initialize_main_thread();
  torrent::initialize();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has("throttle.global_down.max_rate.set_kb"))
    initialize_command_throttle();
}

void
TestCommandThrottle::tearDown() {
  torrent::cleanup();
}

void
TestCommandThrottle::test_global_rate_in_range() {
  call_set("throttle.global_down.max_rate.set_kb", "1024");
  CPPUNIT_ASSERT_EQUAL(uint64_t{1048576}, down_rate());

  call_set("throttle.global_down.max_rate.set_kb", "4194303");
  CPPUNIT_ASSERT_EQUAL(uint64_t{4294966272}, down_rate());

  call_set("throttle.global_down.max_rate.set", "4294966272");
  CPPUNIT_ASSERT_EQUAL(uint64_t{4294966272}, down_rate());
}

void
TestCommandThrottle::test_global_rate_kb_out_of_range() {
  call_set("throttle.global_down.max_rate.set_kb", "1024");

  CPPUNIT_ASSERT_THROW(call_set("throttle.global_down.max_rate.set_kb", "4194304"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(uint64_t{1048576}, down_rate());
}

void
TestCommandThrottle::test_global_rate_bytes_out_of_range() {
  call_set("throttle.global_down.max_rate.set", "1048576");

  CPPUNIT_ASSERT_THROW(call_set("throttle.global_down.max_rate.set", "4294967296"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(uint64_t{1048576}, down_rate());
}

void
TestCommandThrottle::test_global_rate_negative() {
  call_set("throttle.global_down.max_rate.set", "1048576");

  CPPUNIT_ASSERT_THROW(call_set("throttle.global_down.max_rate.set", "-1"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(uint64_t{1048576}, down_rate());
}

void
TestCommandThrottle::test_named_rate_in_range() {
  call_named("throttle.down", "test_named_in_range", "1024");

  auto itr = control->core()->throttles().find("test_named_in_range");

  CPPUNIT_ASSERT(itr != control->core()->throttles().end());
  CPPUNIT_ASSERT_EQUAL(uint64_t{1048576}, itr->second.second->max_rate());
}

void
TestCommandThrottle::test_named_rate_out_of_range() {
  CPPUNIT_ASSERT_THROW(call_named("throttle.down", "test_named_out_of_range", "18014398509481984"), torrent::input_error);
}
