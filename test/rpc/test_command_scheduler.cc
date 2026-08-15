#include "config.h"

#include "test/rpc/test_command_scheduler.h"

#include <chrono>

#include "rpc/command_scheduler.h"
#include "rpc/command_scheduler_item.h"
#include "torrent/object.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandScheduler);

namespace {

const torrent::Object test_command = torrent::Object(std::string("print=scheduled"));

std::chrono::microseconds
time_scheduled(rpc::CommandScheduler& scheduler, const std::string& key) {
  auto itr = scheduler.find(key);

  CPPUNIT_ASSERT(itr != scheduler.end());

  return (*itr)->time_scheduled();
}

}

void
TestCommandScheduler::setUp() {
  TestFixtureWithMainThread::setUp();

  m_main_thread->test_set_cached_time(std::chrono::seconds(0));
}

void
TestCommandScheduler::tearDown() {
  TestFixtureWithMainThread::tearDown();
}

void
TestCommandScheduler::test_parse_rearms_existing_key() {
  rpc::CommandScheduler scheduler;

  scheduler.parse("key", "3600", "3600", test_command);

  auto first = time_scheduled(scheduler, "key");

  m_main_thread->test_add_cached_time(std::chrono::seconds(600));
  scheduler.parse("key", "3600", "3600", test_command);

  CPPUNIT_ASSERT_EQUAL(size_t{1}, scheduler.size());
  CPPUNIT_ASSERT(time_scheduled(scheduler, "key") == first + std::chrono::seconds(600));
}

void
TestCommandScheduler::test_find_locates_a_scheduled_key() {
  rpc::CommandScheduler scheduler;

  CPPUNIT_ASSERT(scheduler.find("key") == scheduler.end());

  scheduler.parse("key", "3600", "3600", test_command);

  CPPUNIT_ASSERT(scheduler.find("key") != scheduler.end());
  CPPUNIT_ASSERT(scheduler.find("other") == scheduler.end());
}
