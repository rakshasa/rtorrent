#include "config.h"

#include "test/rpc/test_parse_commands.h"

#include <chrono>
#include <string>
#include <vector>
#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/utils/log.h>

#include "command_helpers.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestParseCommands);

namespace {

const char* failing_command = "test.parse_commands.fail";
const char* log_name        = "test_parse_commands";

std::chrono::hours first_time{};

torrent::Object
cmd_fail(rpc::target_type, const torrent::Object&) {
  throw torrent::input_error("test input error");
}

std::vector<std::string>
messages(const torrent::log_buffer_ptr& log) {
  std::vector<std::string> result;

  for (const auto& entry : *log)
    result.push_back(entry.message);

  return result;
}

torrent::Object
failing_object() {
  return torrent::Object(std::string(failing_command) + "=");
}

torrent::Object
failing_dict_key() {
  torrent::Object command = torrent::Object::create_dict_key();
  command.as_dict_key() = failing_command;

  return command;
}

}

void
TestParseCommands::setUp() {
  TestFixtureWithMainThread::setUp();

  if (!rpc::commands.has(failing_command))
    CMD2_ANY(failing_command, &cmd_fail);

  first_time += std::chrono::hours(24);
  m_main_thread->test_set_cached_time(first_time);

  m_log = torrent::log_open_log_buffer(log_name);
  torrent::log_add_group_output(torrent::LOG_ERRORS, log_name);
}

void
TestParseCommands::tearDown() {
  m_log.reset();

  TestFixtureWithMainThread::tearDown();
}

void
TestParseCommands::test_nothrow_logs_the_error() {
  torrent::Object result = rpc::call_object_nothrow(failing_object());

  CPPUNIT_ASSERT(result.is_empty());
  CPPUNIT_ASSERT_EQUAL(size_t(1), messages(m_log).size());
  CPPUNIT_ASSERT_EQUAL(std::string("Event command caught input_error: ignored:0 : test input error"), messages(m_log)[0]);
}

void
TestParseCommands::test_object_and_download_wrappers_log() {
  rpc::call_object_nothrow(failing_object());
  CPPUNIT_ASSERT_EQUAL(size_t(1), messages(m_log).size());

  m_main_thread->test_add_cached_time(std::chrono::minutes(1));
  rpc::call_object_d_nothrow(failing_dict_key(), nullptr);
  CPPUNIT_ASSERT_EQUAL(size_t(2), messages(m_log).size());
}

void
TestParseCommands::test_burst_is_limited_and_counted() {
  int ignored = 0;

  rpc::call_object_nothrow(failing_object());

  for (int i = 0; i < 400; i++) {
    rpc::call_object_nothrow(failing_object());
    rpc::call_object_d_nothrow(failing_dict_key(), nullptr);
    ignored += 2;
  }

  CPPUNIT_ASSERT_EQUAL(size_t(1), messages(m_log).size());

  m_main_thread->test_add_cached_time(std::chrono::seconds(59));
  rpc::call_object_nothrow(failing_object());
  ignored++;

  CPPUNIT_ASSERT_EQUAL(size_t(1), messages(m_log).size());

  m_main_thread->test_add_cached_time(std::chrono::seconds(1));
  rpc::call_object_nothrow(failing_object());

  CPPUNIT_ASSERT_EQUAL(size_t(2), messages(m_log).size());
  CPPUNIT_ASSERT_EQUAL("Event command caught input_error: ignored:" + std::to_string(ignored) + " : test input error", messages(m_log)[1]);
}

void
TestParseCommands::test_error_after_limit_is_logged_again() {
  rpc::call_object_nothrow(failing_object());
  rpc::call_object_nothrow(failing_object());

  m_main_thread->test_add_cached_time(std::chrono::minutes(1));
  rpc::call_object_nothrow(failing_object());
  rpc::call_object_nothrow(failing_object());

  CPPUNIT_ASSERT_EQUAL(size_t(2), messages(m_log).size());

  m_main_thread->test_add_cached_time(std::chrono::minutes(1));
  rpc::call_object_nothrow(failing_object());

  CPPUNIT_ASSERT_EQUAL(size_t(3), messages(m_log).size());
  CPPUNIT_ASSERT_EQUAL(std::string("Event command caught input_error: ignored:1 : test input error"), messages(m_log)[1]);
  CPPUNIT_ASSERT_EQUAL(std::string("Event command caught input_error: ignored:1 : test input error"), messages(m_log)[2]);
}
