#include "config.h"

#include "test/src/test_command_tracker.h"

#include <torrent/runtime/network_config.h>
#include <torrent/torrent.h>

#include "control.h"
#include "globals.h"
#include "command_helpers.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandTracker);

void initialize_command_tracker();

static void
call_set(const char* value) {
  rpc::commands.call_command("dht.override_port.set", torrent::Object(std::string(value)));
}

static uint16_t
override_port() {
  return torrent::runtime::network_config()->override_dht_port();
}

void
TestCommandTracker::setUp() {
  torrent::initialize_main_thread();
  torrent::initialize();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has("dht.override_port.set"))
    initialize_command_tracker();
}

void
TestCommandTracker::tearDown() {
  torrent::cleanup();
}

void
TestCommandTracker::test_dht_override_port_in_range() {
  call_set("6881");
  CPPUNIT_ASSERT_EQUAL(uint16_t{6881}, override_port());

  call_set("65535");
  CPPUNIT_ASSERT_EQUAL(uint16_t{65535}, override_port());
}

void
TestCommandTracker::test_dht_override_port_out_of_range() {
  call_set("6881");

  CPPUNIT_ASSERT_THROW(call_set("70000"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(uint16_t{6881}, override_port());

  CPPUNIT_ASSERT_THROW(call_set("-1"), torrent::input_error);
  CPPUNIT_ASSERT_EQUAL(uint16_t{6881}, override_port());
}

void
TestCommandTracker::test_checked_port_value() {
  CPPUNIT_ASSERT_EQUAL(uint16_t{0}, checked_port_value(0, "test"));
  CPPUNIT_ASSERT_EQUAL(uint16_t{6881}, checked_port_value(6881, "test"));
  CPPUNIT_ASSERT_EQUAL(uint16_t{65535}, checked_port_value(65535, "test"));

  CPPUNIT_ASSERT_THROW(checked_port_value(-1, "test"), torrent::input_error);
  CPPUNIT_ASSERT_THROW(checked_port_value(65536, "test"), torrent::input_error);
  CPPUNIT_ASSERT_THROW(checked_port_value(4294967296, "test"), torrent::input_error);
}
