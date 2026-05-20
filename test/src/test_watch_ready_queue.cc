#include "config.h"

#include "test/src/test_watch_ready_queue.h"

#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#include "command_helpers.h"
#include "rpc/command_map.h"
#include "utils/watch_ready_queue.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestWatchReadyQueue);

namespace {

const char* test_load_command = "test.watch_ready.load";
std::vector<std::string> loaded_paths;

torrent::Object
cmd_watch_ready_load([[maybe_unused]] rpc::target_type target, const std::string& path) {
  loaded_paths.push_back(path);
  return torrent::Object();
}

std::string
temporary_path() {
  char path[] = "/tmp/rtorrent-watch-ready-XXXXXX";
  int fd = ::mkstemp(path);

  CPPUNIT_ASSERT(fd != -1);
  CPPUNIT_ASSERT(::close(fd) == 0);
  CPPUNIT_ASSERT(::unlink(path) == 0);

  return path;
}

void
write_file(const std::string& path, const std::string& contents) {
  int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);

  CPPUNIT_ASSERT(fd != -1);
  CPPUNIT_ASSERT(::write(fd, contents.data(), contents.size()) == static_cast<ssize_t>(contents.size()));
  CPPUNIT_ASSERT(::close(fd) == 0);
}

} // namespace

void
TestWatchReadyQueue::setUp() {
  TestFixtureWithMainThread::setUp();

  loaded_paths.clear();
  m_main_thread->test_set_cached_time(std::chrono::seconds(0));

  if (!rpc::commands.has(test_load_command))
    CMD2_ANY_STRING(test_load_command, &cmd_watch_ready_load);
}

void
TestWatchReadyQueue::tearDown() {
  loaded_paths.clear();

  TestFixtureWithMainThread::tearDown();
}

void
TestWatchReadyQueue::test_repeated_events_reset_quiet_time() {
  auto path = temporary_path();
  write_file(path, "torrent");

  utils::WatchReadyQueue queue;
  queue.push(test_load_command, path);

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(300));
  queue.push(test_load_command, path);

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(300));
  m_main_thread->test_process_events_without_cached_time();
  CPPUNIT_ASSERT(loaded_paths.empty());

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(201));
  m_main_thread->test_process_events_without_cached_time();
  CPPUNIT_ASSERT(loaded_paths.size() == 1);
  CPPUNIT_ASSERT(loaded_paths.front() == path);

  CPPUNIT_ASSERT(::unlink(path.c_str()) == 0);
}

void
TestWatchReadyQueue::test_retry_restats_zero_length_file_without_new_event() {
  auto path = temporary_path();
  write_file(path, "");

  utils::WatchReadyQueue queue;
  queue.push(test_load_command, path);

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(251));
  m_main_thread->test_process_events_without_cached_time();
  CPPUNIT_ASSERT(loaded_paths.empty());

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(10));
  // No second queue event: retry processing must re-stat the same pending path.
  write_file(path, "torrent");

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(240));
  m_main_thread->test_process_events_without_cached_time();
  CPPUNIT_ASSERT(loaded_paths.empty());

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(500));
  m_main_thread->test_process_events_without_cached_time();
  CPPUNIT_ASSERT(loaded_paths.size() == 1);
  CPPUNIT_ASSERT(loaded_paths.front() == path);

  CPPUNIT_ASSERT(::unlink(path.c_str()) == 0);
}

void
TestWatchReadyQueue::test_unrelated_events_do_not_delay_missing_retry() {
  auto path = temporary_path();
  auto unrelated_path = temporary_path();

  utils::WatchReadyQueue queue;
  queue.push(test_load_command, path);

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(100));
  queue.push(test_load_command, unrelated_path);

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(160));
  write_file(path, "torrent");
  m_main_thread->test_process_events_without_cached_time();
  CPPUNIT_ASSERT(loaded_paths.empty());

  m_main_thread->test_add_cached_time(std::chrono::milliseconds(501));
  m_main_thread->test_process_events_without_cached_time();
  CPPUNIT_ASSERT(loaded_paths.size() == 1);
  CPPUNIT_ASSERT(loaded_paths.front() == path);

  CPPUNIT_ASSERT(::unlink(path.c_str()) == 0);
}

void
TestWatchReadyQueue::test_missing_paths_expire_without_dispatch() {
  auto path = temporary_path();

  utils::WatchReadyQueue queue;
  queue.push(test_load_command, path);

  m_main_thread->test_add_cached_time(std::chrono::seconds(10) + std::chrono::milliseconds(1));
  m_main_thread->test_process_events_without_cached_time();

  CPPUNIT_ASSERT(loaded_paths.empty());
  CPPUNIT_ASSERT(queue.empty());
}

void
TestWatchReadyQueue::test_shutdown_discards_pending_loads() {
  auto path = temporary_path();
  write_file(path, "torrent");

  utils::WatchReadyQueue queue;
  queue.push(test_load_command, path);
  queue.shutdown();

  m_main_thread->test_add_cached_time(std::chrono::seconds(1));
  m_main_thread->test_process_events_without_cached_time();

  CPPUNIT_ASSERT(loaded_paths.empty());
  CPPUNIT_ASSERT(queue.empty());

  CPPUNIT_ASSERT(::unlink(path.c_str()) == 0);
}
