#include "config.h"

#include "test/src/test_command_local.h"

#include <torrent/torrent.h>
#include <torrent/runtime/socket_manager.h>
#include <torrent/utils/option_strings.h>

#include "control.h"
#include "globals.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandLocal);

void initialize_command_local();

void
TestCommandLocal::setUp() {
  torrent::initialize_main_thread();
  torrent::initialize();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has("system.sockets.size"))
    initialize_command_local();
}

void
TestCommandLocal::tearDown() {
  torrent::cleanup();
}

void
TestCommandLocal::test_socket_category_commands() {
  for (uint32_t i = 0; i < torrent::runtime::SocketManager::category_count; ++i) {
    auto category = static_cast<torrent::runtime::socket_manager_category_t>(i);
    auto name     = "system.sockets." + torrent::option_to_str_or_throw(torrent::OPTION_SOCKET_CATEGORY, i);

    for (const auto suffix : {".size", ".max_size", ".min_alloc", ".max_alloc"})
      if (rpc::commands.has(name + suffix))
        CPPUNIT_ASSERT_NO_THROW(rpc::commands.call(name + suffix));

    const bool has_allocation = category != torrent::runtime::category_generic;

    CPPUNIT_ASSERT(rpc::commands.has(name + ".size"));
    CPPUNIT_ASSERT(rpc::commands.has(name + ".max_size"));
    CPPUNIT_ASSERT(rpc::commands.has(name + ".min_alloc"));

    CPPUNIT_ASSERT_EQUAL(has_allocation, rpc::commands.has(name + ".max_alloc"));
    CPPUNIT_ASSERT_EQUAL(has_allocation, rpc::commands.has(name + ".min_alloc.set"));
    CPPUNIT_ASSERT_EQUAL(has_allocation, rpc::commands.has(name + ".max_alloc.set"));
  }
}
