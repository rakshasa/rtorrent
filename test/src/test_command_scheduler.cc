#include "config.h"

#include "test/src/test_command_scheduler.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>
#include <torrent/data/download_data.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/system/scheduler.h>
#include <torrent/system/thread.h>
#include <torrent/torrent.h>
#include <torrent/utils/log.h>
#include <torrent/utils/log_buffer.h>

#include "command_helpers.h"
#include "control.h"
#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "core/view.h"
#include "core/view_manager.h"
#include "globals.h"
#include "rpc/parse_commands.h"
#include "session/thread_session.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestSchedulerSimple);

void initialize_command_download();
void initialize_command_scheduler();
void initialize_command_system();
void initialize_command_ui();

namespace {

struct SchedulerAccess : torrent::system::Scheduler {
  using torrent::system::Scheduler::perform;
};

struct ThreadAccess : torrent::system::Thread {
  using torrent::system::Thread::set_cached_time;
};

const char* resumed_command = "event.download.resumed";
const char* hook_command    = "test.scheduler_simple.hook";
const char* log_name        = "test_scheduler_simple";

const char* changed_message = "Scheduler stopped: an event handler changed the started view.";
const char* shrink_message  = "Scheduler stopped: an event handler prevented the active view from shrinking.";

std::vector<core::Download*> downloads;

core::View* active;
core::View* started;

void (*on_resumed)(core::Download*);
void (*on_hook)(core::Download*);

int created;

std::string nested_refusal;

std::chrono::minutes log_advance;

std::string temp_dir;

core::Download*
new_download() {
  torrent::Object info = torrent::Object::create_map();
  info.insert_key("name", "d" + std::to_string(created++));
  info.insert_key("length", int64_t{16});
  info.insert_key("piece length", int64_t{262144});
  info.insert_key("pieces", std::string(20, char(0)));

  torrent::Object rtorrent = torrent::Object::create_map();
  rtorrent.insert_key("hashing", int64_t{0});
  rtorrent.insert_key("priority", int64_t{2});
  rtorrent.insert_key("state_changed", int64_t{0});
  rtorrent.insert_key("state_counter", int64_t{0});
  rtorrent.insert_key("connection_leech", std::string("leech"));
  rtorrent.insert_key("choke_heuristics.up.leech", std::string("upload_leech"));
  rtorrent.insert_key("choke_heuristics.down.leech", std::string("download_leech"));

  auto object = new torrent::Object(torrent::Object::create_map());
  object->insert_key("info", info);
  object->insert_key("rtorrent", rtorrent);

  return new core::Download(torrent::download_add(object, 0));
}

void
mark_hash_checked(core::Download* download) {
  download->download()->open();
  download->download()->set_bitfield(false);

  auto callback = download->data()->slot_initial_hash();
  download->data()->slot_initial_hash() = [] {};

  CPPUNIT_ASSERT(download->download()->hash_check(true));

  auto perform = &SchedulerAccess::perform;
  (torrent::this_thread::scheduler()->*perform)(torrent::this_thread::cached_time());

  download->data()->slot_initial_hash() = callback;
  CPPUNIT_ASSERT(download->is_hash_checked());
}

void
add_downloads(int count) {
  for (int i = 0; i < count; i++) {
    auto download = new_download();

    control->core()->download_list()->insert(download);
    download->set_directory(temp_dir);
    mark_hash_checked(download);

    downloads.push_back(download);
  }
}

void
add_waiting_downloads(int count) {
  add_downloads(count);

  for (auto download : downloads)
    active->set_not_visible(download);
}

std::string
names(const core::View& view) {
  std::string result;

  for (auto itr = view.begin_visible(); itr != view.end_visible(); ++itr) {
    if (!result.empty())
      result += ',';

    result += itr->get()->info()->name().c_str();
  }

  return result;
}

std::string
refusal(const char* command) {
  try {
    rpc::call_command(command, torrent::Object(), rpc::make_target(downloads.front()));
  } catch (const torrent::input_error& e) {
    return e.what();
  }

  return std::string();
}

void
set_max_active(int64_t value) {
  rpc::call_command("scheduler.max_active.set", value);
}

torrent::Object
cmd_resumed(core::Download* download, const torrent::Object&) {
  if (on_resumed != nullptr)
    on_resumed(download);

  return torrent::Object();
}

torrent::Object
cmd_hook(core::Download* download, const torrent::Object&) {
  if (on_hook != nullptr)
    on_hook(download);

  return torrent::Object();
}

void
erase_current(core::Download* download) {
  auto entry = *started->begin_visible();

  control->core()->download_list()->erase_ptr(download);
  started->insert(entry);
  started->set_visible(download);
}

void
hide_last(core::Download*) {
  started->set_not_visible(downloads.back());
}

void
rotate_started(core::Download*) {
  auto first = started->begin_visible()->get();

  started->set_not_visible(first);
  started->set_visible(first);
}

void
add_hidden_entries(core::Download*) {
  for (int i = 0; i < 64; i++) {
    started->insert(nullptr);
  }
}

void
restore_to_active(core::Download* download) {
  active->set_visible(download);
}

void
update_again(core::Download*) {
  nested_refusal = refusal("scheduler.simple.update");
}

void
insert_empty_slot() {
  started->insert(nullptr);
  started->set_visible(nullptr);
}

}

void
TestSchedulerSimple::setUp() {
  mock_redirect_defaults();

  torrent::initialize_main_thread();
  torrent::initialize();
  session::ThreadSession::create_thread();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has("d.name"))
    initialize_command_download();

  if (!rpc::commands.has("view.set_visible"))
    initialize_command_ui();

  if (!rpc::commands.has("scheduler.max_active"))
    initialize_command_scheduler();

  if (!rpc::commands.has("system.file.allocate"))
    initialize_command_system();

  if (!rpc::commands.has(resumed_command))
    CMD2_DL(resumed_command, &cmd_resumed);

  if (!rpc::commands.has(hook_command))
    CMD2_DL(hook_command, &cmd_hook);

  downloads.clear();
  on_resumed = nullptr;
  on_hook    = nullptr;
  created    = 0;

  char path[] = "/tmp/rtorrent_test_scheduler_XXXXXX";
  CPPUNIT_ASSERT(mkdtemp(path) != nullptr);
  temp_dir = path;

  active  = *control->view_manager()->insert("active");
  started = *control->view_manager()->insert("started");

  set_max_active(10);
}

void
TestSchedulerSimple::tearDown() {
  on_resumed = nullptr;
  on_hook    = nullptr;

  control->core()->download_list()->clear();
  control->view_manager()->clear();

  session::ThreadSession::destroy_thread();
  torrent::cleanup();
  std::filesystem::remove_all(temp_dir);
}

void
TestSchedulerSimple::test_update_refuses_when_resumed_erases_current() {
  add_waiting_downloads(1);
  on_resumed = &erase_current;

  CPPUNIT_ASSERT_EQUAL(std::string(changed_message), refusal("scheduler.simple.update"));
}

void
TestSchedulerSimple::test_update_refuses_when_resumed_reorders_started() {
  add_waiting_downloads(3);
  on_resumed = &rotate_started;

  CPPUNIT_ASSERT_EQUAL(std::string(changed_message), refusal("scheduler.simple.update"));
  CPPUNIT_ASSERT_EQUAL(std::string("d1,d2,d0"), names(*started));
  CPPUNIT_ASSERT_EQUAL(std::string("d0"), names(*active));
}

void
TestSchedulerSimple::test_removed_refuses_when_resumed_hides_other() {
  add_waiting_downloads(3);
  on_resumed = &hide_last;

  CPPUNIT_ASSERT_EQUAL(std::string(changed_message), refusal("scheduler.simple.removed"));
  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1"), names(*started));
}

void
TestSchedulerSimple::test_full_limit_ignores_started_changes() {
  add_waiting_downloads(3);
  set_max_active(1);
  on_resumed = &hide_last;

  CPPUNIT_ASSERT_EQUAL(std::string(), refusal("scheduler.simple.update"));
  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1"), names(*started));
  CPPUNIT_ASSERT_EQUAL(std::string("d0"), names(*active));
}

void
TestSchedulerSimple::test_hidden_entries_do_not_refuse() {
  add_waiting_downloads(3);
  on_resumed = &add_hidden_entries;

  CPPUNIT_ASSERT_EQUAL(std::string(), refusal("scheduler.simple.update"));
  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1,d2"), names(*active));
}

void
TestSchedulerSimple::test_update_skips_empty_slot() {
  add_waiting_downloads(1);
  refusal("scheduler.simple.update");
  insert_empty_slot();
  on_hook = &update_again;
  nested_refusal = "update did not run";

  core::View::base_type matches;
  started->filter_by(torrent::Object(std::string(hook_command) + "="), matches);

  CPPUNIT_ASSERT_EQUAL(std::string(), nested_refusal);
}

void
TestSchedulerSimple::test_removed_skips_empty_slot() {
  insert_empty_slot();
  add_waiting_downloads(2);
  set_max_active(1);

  CPPUNIT_ASSERT_EQUAL(std::string(), refusal("scheduler.simple.removed"));
  CPPUNIT_ASSERT_EQUAL(std::string("d0"), names(*active));
}

void
TestSchedulerSimple::test_pause_refuses_when_handler_restores_active() {
  add_downloads(3);
  set_max_active(1);
  active->set_event_removed(torrent::Object(std::string(hook_command) + "="));
  on_hook = &restore_to_active;

  CPPUNIT_ASSERT_EQUAL(std::string(shrink_message), refusal("scheduler.simple.update"));
  CPPUNIT_ASSERT_EQUAL(std::string("d1,d2,d0"), names(*active));
}

void
TestSchedulerSimple::test_unlimited_and_zero_limits() {
  add_waiting_downloads(3);
  set_max_active(-1);

  CPPUNIT_ASSERT_EQUAL(std::string(), refusal("scheduler.simple.update"));
  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1,d2"), names(*active));

  CPPUNIT_ASSERT_EQUAL(std::string(), refusal("scheduler.simple.removed"));
  CPPUNIT_ASSERT_EQUAL(std::string("d1,d2"), names(*active));

  set_max_active(0);

  CPPUNIT_ASSERT_EQUAL(std::string(), refusal("scheduler.simple.update"));
  CPPUNIT_ASSERT_EQUAL(std::string(), names(*active));
}

void
TestSchedulerSimple::test_refusal_from_started_event_is_logged() {
  add_waiting_downloads(3);
  on_resumed = &hide_last;
  started->set_event_added(torrent::Object("scheduler.simple.update="));

  auto log = torrent::log_open_log_buffer(log_name);
  torrent::log_add_group_output(torrent::LOG_ERRORS, log_name);

  auto set_cached_time = &ThreadAccess::set_cached_time;
  log_advance += std::chrono::minutes(2);
  (torrent::this_thread::thread()->*set_cached_time)(torrent::this_thread::cached_time() + log_advance);

  started->set_not_visible(downloads[1]);
  started->set_visible(downloads[1]);

  CPPUNIT_ASSERT_EQUAL(size_t(1), log->size());
  CPPUNIT_ASSERT(log->front().message.starts_with("Event command caught input_error: ignored:"));
  CPPUNIT_ASSERT(log->front().message.ends_with(" : " + std::string(changed_message)));
}
