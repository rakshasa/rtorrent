#include "config.h"

#include "test/src/test_command_events.h"

#include <functional>
#include <map>
#include <string>
#include <torrent/data/download_data.h>
#include <torrent/download_info.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/torrent.h>
#include <torrent/system/scheduler.h>
#include <torrent/utils/log.h>
#include <vector>

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

CPPUNIT_TEST_SUITE_REGISTRATION(TestCommandEvents);

void initialize_command_download();
void initialize_command_events();
void initialize_command_scheduler();
void initialize_command_system();
void initialize_command_ui();

namespace {

struct SchedulerAccess : torrent::system::Scheduler {
  using torrent::system::Scheduler::perform;
};

const char*                                             event_command = "test.command_events.event";
const char*                                             existing_file = ".";
const char*                                             missing_file  = "test_command_events.missing";

std::string                                             handled;
std::string                                             captured_log;
std::function<void(core::Download*)>                    opened_action;
std::function<void(core::Download*)>                    queued_action;
std::function<void(core::Download*)>                    event_action;
int                                                     hash_failed_events;
int                                                     closed_events;

std::map<core::Download*, std::vector<core::Download*>> erase_on;

torrent::Object
cmd_event(core::Download* download, const torrent::Object&) {
  if (download->is_erasing())
    return torrent::Object();

  if (!handled.empty())
    handled += ',';

  handled += download->info()->name().c_str();

  if (event_action)
    event_action(download);

  for (auto target : erase_on[download])
    control->core()->download_list()->erase_ptr(target);

  return torrent::Object();
}

torrent::Object
cmd_queued(core::Download* download, const torrent::Object&) {
  if (queued_action)
    queued_action(download);
  return torrent::Object();
}

torrent::Object
cmd_opened(core::Download* download, const torrent::Object&) {
  if (opened_action)
    opened_action(download);
  return torrent::Object();
}

torrent::Object
cmd_failing_event(core::Download* download, const torrent::Object& args) {
  cmd_event(download, args);
  throw torrent::storage_error("controlled handler failure");
}

torrent::Object
cmd_hash_failed(core::Download*, const torrent::Object&) {
  hash_failed_events++;
  return torrent::Object();
}

torrent::Object
cmd_closed(core::Download*, const torrent::Object&) {
  closed_events++;
  return torrent::Object();
}

std::vector<core::Download*>
insert_downloads(int64_t state, const std::string& tied_to_file, core::DownloadList* list = nullptr, int first = 0, int count = 5) {
  if (list == nullptr)
    list = control->core()->download_list();
  std::vector<core::Download*> downloads;

  for (int i = first; i < first + count; i++) {
    torrent::Object info = torrent::Object::create_map();
    info.insert_key("name", "d" + std::to_string(i));
    info.insert_key("length", int64_t{16});
    info.insert_key("piece length", int64_t{262144});
    info.insert_key("pieces", std::string(20, char(0)));

    torrent::Object rtorrent = torrent::Object::create_map();
    rtorrent.insert_key("state", state);
    rtorrent.insert_key("complete", int64_t{0});
    rtorrent.insert_key("tied_to_file", tied_to_file);
    rtorrent.insert_key("ignore_commands", int64_t{0});
    rtorrent.insert_key("hashing", int64_t{1});

    auto object = new torrent::Object(torrent::Object::create_map());
    object->insert_key("info", info);
    object->insert_key("rtorrent", rtorrent);

    auto download = new core::Download(torrent::download_add(object, 0));
    list->insert(download);
    downloads.push_back(download);
  }

  return downloads;
}

core::View*
insert_hidden_view(const std::string& name, const std::vector<core::Download*>& downloads) {
  auto view = *control->view_manager()->insert(name);

  for (auto download : downloads)
    view->set_not_visible(download);

  return view;
}

void
mark_hash_checked(core::Download* download, bool complete) {
  download->download()->open();
  download->download()->set_bitfield(complete);
  auto callback = download->data()->slot_initial_hash();
  download->data()->slot_initial_hash() = [] {};
  CPPUNIT_ASSERT(download->download()->hash_check(true));
  auto perform = &SchedulerAccess::perform;
  (torrent::this_thread::scheduler()->*perform)(torrent::this_thread::cached_time());
  download->data()->slot_initial_hash() = callback;
  CPPUNIT_ASSERT(download->is_hash_checked());
  CPPUNIT_ASSERT_EQUAL(complete, download->is_done());
}

void
fail_hash_check(core::Download* download) {
  auto hash_done = download->data()->slot_initial_hash();
  hash_done();
}

void
check_erasing_handlers(const char* command, const std::vector<core::Download*>& d) {
  erase_on = {{d[0], {d[0]}}, {d[1], {d[2], d[4]}}};

  rpc::call_command(command, torrent::Object());

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);

  rpc::call_command(command, torrent::Object());

  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1,d3"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(2), control->core()->download_list()->size());
}

} // namespace

void
TestCommandEvents::setUp() {
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

  if (!rpc::commands.has("start_tied"))
    initialize_command_events();

  if (!rpc::commands.has("scheduler.max_active"))
    initialize_command_scheduler();

  if (!rpc::commands.has("system.file.allocate"))
    initialize_command_system();

  if (!rpc::commands.has(event_command)) {
    CMD2_DL(event_command, &cmd_event);
    CMD2_DL("event.download.hash_removed", &cmd_event);
    CMD2_DL("event.download.hash_failed", &cmd_hash_failed);
    CMD2_DL("event.download.closed", &cmd_closed);
    CMD2_DL("event.download.opened", &cmd_opened);
    CMD2_DL("event.download.hash_queued", &cmd_queued);
    CMD2_DL("test.command_events.failing_event", &cmd_failing_event);
  }

  handled.clear();
  captured_log.clear();
  opened_action = {};
  queued_action = {};
  event_action = {};
  hash_failed_events = 0;
  closed_events = 0;
  erase_on.clear();
}

void
TestCommandEvents::tearDown() {
  event_action = {};
  erase_on.clear();
  torrent::log_close_output("test_events_log");

  control->core()->download_list()->clear();
  control->view_manager()->clear();

  session::ThreadSession::destroy_thread();
  torrent::cleanup();
}

void
TestCommandEvents::test_start_tied_with_erasing_handlers() {
  auto downloads = insert_downloads(0, existing_file);

  insert_hidden_view("started", downloads)->set_event_added(torrent::Object(std::string(event_command) + "="));

  check_erasing_handlers("start_tied", downloads);
}

void
TestCommandEvents::test_stop_untied_with_erasing_handlers() {
  auto downloads = insert_downloads(1, missing_file);

  insert_hidden_view("stopped", downloads)->set_event_added(torrent::Object(std::string(event_command) + "="));

  check_erasing_handlers("stop_untied", downloads);
}

void
TestCommandEvents::test_close_untied_with_erasing_handlers() {
  auto downloads = insert_downloads(1, missing_file);

  insert_hidden_view("stopped", downloads);
  control->view_manager()->insert("active");

  for (auto download : downloads) {
    download->download()->open();
    rpc::call_command("d.hashing.set", int64_t{0}, rpc::make_target(download));
  }

  check_erasing_handlers("close_untied", downloads);
}

void
TestCommandEvents::test_hash_failure_releases_active() {
  auto downloads = insert_downloads(1, missing_file);
  auto active    = *control->view_manager()->insert("active");

  fail_hash_check(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(1, hash_failed_events);
  CPPUNIT_ASSERT_EQUAL(size_t(4), active->size());
}

void
TestCommandEvents::test_hash_failure_lets_scheduler_start_another() {
  auto downloads = insert_downloads(1, missing_file);
  auto active    = insert_hidden_view("active", {downloads[1], downloads[2], downloads[3], downloads[4]});

  control->view_manager()->insert("started");
  active->set_event_removed(torrent::Object("scheduler.simple.update="));
  rpc::call_command_set_value("scheduler.max_active.set", 1);

  fail_hash_check(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(size_t(1), active->size());
  CPPUNIT_ASSERT_EQUAL(downloads[1], active->begin_visible()->get());
}

void
TestCommandEvents::test_hash_failure_with_erasing_handler() {
  auto downloads = insert_downloads(1, missing_file);
  auto active    = *control->view_manager()->insert("active");

  active->set_event_removed(torrent::Object(std::string(event_command) + "="));
  erase_on = {{downloads[0], {downloads[0]}}};

  fail_hash_check(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(0, hash_failed_events);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_resume_with_erasing_handler() {
  auto downloads = insert_downloads(1, missing_file);

  insert_hidden_view("active", downloads)->set_event_added(torrent::Object(std::string(event_command) + "="));
  erase_on = {{downloads[0], {downloads[0]}}};

  control->core()->download_list()->resume(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_resume_with_erasing_and_failing_handler() {
  auto downloads = insert_downloads(1, missing_file);

  insert_hidden_view("active", downloads)->set_event_added(torrent::Object("test.command_events.failing_event="));
  erase_on = {{downloads[0], {downloads[0]}}};
  torrent::log_open_output("test_events_log", [](const char* data, size_t size, int) { captured_log.append(data, size); });
  torrent::log_add_group_output(torrent::LOG_TORRENT_EVENTS, "test_events_log");

  control->core()->download_list()->resume(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
  CPPUNIT_ASSERT(captured_log.find("could not resume download : controlled handler failure") != std::string::npos);
}

void
TestCommandEvents::test_pause_with_erasing_handler() {
  auto downloads = insert_downloads(1, missing_file);
  auto view      = *control->view_manager()->insert("active");

  view->set_event_removed(torrent::Object(std::string(event_command) + "="));
  erase_on = {{downloads[0], {downloads[0]}}};

  control->core()->download_list()->pause(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_pause_with_erasing_and_failing_handler() {
  auto downloads = insert_downloads(1, missing_file);
  auto view      = *control->view_manager()->insert("active");

  view->set_event_removed(torrent::Object("test.command_events.failing_event="));
  erase_on = {{downloads[0], {downloads[0]}}};
  torrent::log_open_output("test_events_log", [](const char* data, size_t size, int) { captured_log.append(data, size); });
  torrent::log_add_group_output(torrent::LOG_TORRENT_EVENTS, "test_events_log");

  control->core()->download_list()->pause(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
  CPPUNIT_ASSERT(captured_log.find("could not pause download : controlled handler failure") != std::string::npos);
}

void
TestCommandEvents::test_close_directly_with_erasing_handler() {
  auto downloads = insert_downloads(1, missing_file);

  insert_hidden_view("stopped", downloads)->set_event_added(torrent::Object(std::string(event_command) + "="));
  erase_on = {{downloads[0], {downloads[0]}}};

  control->core()->download_list()->close_directly(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_close_quick_with_erasing_handler() {
  auto downloads = insert_downloads(1, missing_file);

  control->view_manager()->insert("active");
  erase_on = {{downloads[0], {downloads[0]}}};

  control->core()->download_list()->close_quick(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_shutdown_with_erasing_handlers() {
  auto downloads = insert_downloads(1, missing_file);

  control->view_manager()->insert("active");
  erase_on = {{downloads[1], {downloads[1]}}, {downloads[3], {downloads[2], downloads[3]}}};

  control->core()->shutdown(false);

  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1,d2,d3,d4"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(2), control->core()->download_list()->size());
}

void
TestCommandEvents::test_close_directly_open_with_erasing_handler() {
  auto downloads = insert_downloads(1, missing_file);
  downloads[0]->download()->open();
  insert_hidden_view("stopped", downloads)->set_event_added(torrent::Object(std::string(event_command) + "="));
  erase_on = {{downloads[0], {downloads[0]}}};

  control->core()->download_list()->close_directly(downloads[0]);

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_quick_shutdown_with_erasing_handlers() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  erase_on = {{downloads[1], {downloads[1]}}, {downloads[3], {downloads[2], downloads[3]}}};

  control->core()->shutdown(true);

  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1,d2,d3,d4"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(2), control->core()->download_list()->size());
}

void
TestCommandEvents::test_clear_refuses_erasing_handlers() {
  core::Manager manager;
  auto fresh = insert_downloads(1, missing_file, manager.download_list(), 0, 1);

  manager.download_list()->erase_ptr(fresh[0]);

  CPPUNIT_ASSERT(manager.download_list()->empty());

  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  erase_on = {{downloads[4], {downloads[3], downloads[4]}}};

  control->core()->download_list()->clear();

  CPPUNIT_ASSERT_EQUAL(std::string("d4,d3,d2,d1,d0"), handled);
  CPPUNIT_ASSERT(control->core()->download_list()->empty());

  auto later = insert_downloads(1, missing_file, control->core()->download_list(), 5, 1);
  erase_on.clear();
  control->core()->download_list()->erase_ptr(later[0]);

  CPPUNIT_ASSERT(control->core()->download_list()->empty());
}

void
TestCommandEvents::test_clear_keeps_downloads_for_handlers() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  size_t      size = 0;
  std::string name;
  event_action = [&](core::Download* download) {
    if (download != downloads[3])
      return;

    size = control->core()->download_list()->size();
    name = rpc::call_command_string("d.name", rpc::make_target(downloads[4]));
  };

  control->core()->download_list()->clear();

  torrent::DList remaining;
  torrent::download_list(remaining);

  CPPUNIT_ASSERT(remaining.empty());
  CPPUNIT_ASSERT_EQUAL(size_t(5), size);
  CPPUNIT_ASSERT_EQUAL(std::string("d4"), name);
  CPPUNIT_ASSERT(control->core()->download_list()->empty());
}

void
TestCommandEvents::test_clear_closes_the_rest_after_failed_close() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  downloads[2]->download()->open();
  event_action = [&](core::Download* download) {
    if (download == downloads[2])
      rpc::call_command_set_value("d.hashing.set", int64_t{1}, rpc::make_target(download));
  };

  CPPUNIT_ASSERT_THROW(control->core()->download_list()->clear(), torrent::internal_error);

  CPPUNIT_ASSERT_EQUAL(std::string("d4,d3,d2,d1,d0"), handled);
  CPPUNIT_ASSERT(control->core()->download_list()->empty());
}

void
TestCommandEvents::test_hashing_pass_stops_after_view_change() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  auto downloads = insert_downloads(1, missing_file, manager.download_list());
  manager.set_hashing_view(hashing);
  opened_action = [&](core::Download* download) {
    if (download == downloads[0])
      manager.download_list()->erase_ptr(downloads[2]);
  };

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT_MESSAGE(downloads[0]->message(), downloads[0]->is_open());
  CPPUNIT_ASSERT(!downloads[1]->is_open());
  CPPUNIT_ASSERT_EQUAL(size_t(4), manager.download_list()->size());
  downloads[0]->download()->close();
  manager.download_list()->clear();
}

void
TestCommandEvents::test_hashing_pass_stops_after_same_size_view_change() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  auto downloads = insert_downloads(1, missing_file, manager.download_list());
  hashing->set_not_visible(downloads[4]);
  manager.set_hashing_view(hashing);
  opened_action = [&](core::Download* download) {
    if (download == downloads[0]) {
      hashing->set_not_visible(downloads[0]);
      hashing->set_visible(downloads[4]);
    }
  };

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT_MESSAGE(downloads[0]->message(), downloads[0]->is_open());
  CPPUNIT_ASSERT(!downloads[1]->is_open());
  CPPUNIT_ASSERT(!downloads[1]->is_hash_failed());
  CPPUNIT_ASSERT_EQUAL(size_t(4), hashing->size_visible());
  downloads[0]->download()->close();
  manager.download_list()->clear();
}

void
TestCommandEvents::test_hashing_pass_logs_failure_after_view_change() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  auto downloads = insert_downloads(1, missing_file, manager.download_list());
  manager.set_hashing_view(hashing);
  torrent::log_open_output("test_events_log", [](const char* data, size_t size, int) { captured_log.append(data, size); });
  torrent::log_add_group_output(torrent::LOG_TORRENT_EVENTS, "test_events_log");
  opened_action = [&](core::Download* download) {
    if (download == downloads[0]) {
      manager.download_list()->erase_ptr(downloads[2]);
      throw torrent::storage_error("controlled hashing failure");
    }
  };

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT(!downloads[1]->is_open());
  CPPUNIT_ASSERT_MESSAGE(captured_log, captured_log.find("hashing failed : controlled hashing failure") != std::string::npos);
  downloads[0]->download()->close();
  manager.download_list()->clear();
}

void
TestCommandEvents::test_hashing_pass_skips_downloads_removed_by_callbacks() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  auto downloads = insert_downloads(1, missing_file, manager.download_list());
  manager.set_hashing_view(hashing);

  torrent::this_thread::thread()->callback_interrupt([&]() {
      manager.download_list()->erase_ptr(downloads[1]);
      hashing->set_not_visible(downloads[2]);
    });

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT(downloads[0]->is_open());
  CPPUNIT_ASSERT(!downloads[2]->is_open());
  CPPUNIT_ASSERT(downloads[3]->is_open());
  CPPUNIT_ASSERT(downloads[4]->is_open());
  CPPUNIT_ASSERT_EQUAL(size_t(4), manager.download_list()->size());

  for (auto download : {downloads[0], downloads[3], downloads[4]})
    download->download()->close();

  manager.download_list()->clear();
}

void
TestCommandEvents::test_hashing_pass_runs_callbacks_between_downloads() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  auto downloads = insert_downloads(1, missing_file, manager.download_list());
  manager.set_hashing_view(hashing);
  opened_action = [&](core::Download* download) {
    if (download == downloads[0])
      torrent::this_thread::thread()->callback_interrupt([&]() { manager.download_list()->erase_ptr(downloads[2]); });
  };

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT(downloads[0]->is_open());
  CPPUNIT_ASSERT(downloads[1]->is_open());
  CPPUNIT_ASSERT(downloads[3]->is_open());
  CPPUNIT_ASSERT(downloads[4]->is_open());
  CPPUNIT_ASSERT_EQUAL(size_t(4), manager.download_list()->size());

  for (auto download : {downloads[0], downloads[1], downloads[3], downloads[4]})
    download->download()->close();

  manager.download_list()->clear();
}

void
TestCommandEvents::test_hashing_pass_leaves_added_downloads_for_the_next_call() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  auto downloads = insert_downloads(1, missing_file, manager.download_list(), 0, 2);
  manager.set_hashing_view(hashing);

  core::Download* added = nullptr;

  torrent::this_thread::thread()->callback_interrupt([&]() {
      added = insert_downloads(1, missing_file, manager.download_list(), 2, 1)[0];
    });

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT(added != nullptr);
  CPPUNIT_ASSERT(downloads[0]->is_open());
  CPPUNIT_ASSERT(downloads[1]->is_open());
  CPPUNIT_ASSERT(!added->is_open());

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT(added->is_open());

  for (auto download : {downloads[0], downloads[1], added})
    download->download()->close();

  manager.download_list()->clear();
}

void
TestCommandEvents::test_quick_shutdown_closes_open_download() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  downloads[0]->download()->open();

  control->core()->shutdown(true);

  CPPUNIT_ASSERT(!downloads[0]->is_open());
}

void
TestCommandEvents::test_hash_done_stops_after_stopped_handler_erases() {
  auto downloads = insert_downloads(1, missing_file);
  mark_hash_checked(downloads[0], false);
  rpc::call_command("d.complete.set", int64_t{1}, rpc::make_target(downloads[0]));
  insert_hidden_view("stopped", downloads)->set_event_added(torrent::Object(std::string(event_command) + "="));
  erase_on = {{downloads[0], {downloads[0]}}};

  auto hash_done = downloads[0]->data()->slot_initial_hash();
  hash_done();

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_meta_download_stops_after_stop_handler_erases() {
  auto downloads = insert_downloads(1, missing_file);
  mark_hash_checked(downloads[0], true);
  const_cast<torrent::DownloadInfo*>(downloads[0]->info())->set_flags(torrent::DownloadInfo::flag_meta_download);
  insert_hidden_view("stopped", downloads)->set_event_added(torrent::Object(std::string(event_command) + "="));
  control->view_manager()->insert("active");
  erase_on = {{downloads[0], {downloads[0]}}};

  auto hash_done = downloads[0]->data()->slot_initial_hash();
  hash_done();

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_meta_download_stops_after_close_handler_erases() {
  auto downloads = insert_downloads(1, missing_file);
  mark_hash_checked(downloads[0], true);
  const_cast<torrent::DownloadInfo*>(downloads[0]->info())->set_flags(torrent::DownloadInfo::flag_meta_download);
  insert_hidden_view("stopped", downloads);
  control->view_manager()->insert("active");
  erase_on = {{downloads[0], {downloads[0]}}};

  auto hash_done = downloads[0]->data()->slot_initial_hash();
  hash_done();

  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_hashing_pass_stops_after_reallocated_view() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  control->view_manager()->insert("active");
  auto downloads = insert_downloads(1, missing_file, manager.download_list());
  manager.set_hashing_view(hashing);
  auto storage = [&] { return static_cast<const void*>(&*hashing->begin_visible()); };
  auto original_storage = storage();
  bool moved = false;
  opened_action = [&](core::Download* download) {
    if (download == downloads[0]) {
      manager.download_list()->erase_ptr(download);
      std::vector<core::Download*> replacements;
      while (storage() == original_storage && replacements.size() < 4096)
        replacements.push_back(insert_downloads(1, missing_file, manager.download_list(), 100 + replacements.size(), 1)[0]);
      moved = storage() != original_storage;
      for (size_t i = 1; i < replacements.size(); i++)
        manager.download_list()->erase_ptr(replacements[i]);
    }
  };

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT(moved);
  CPPUNIT_ASSERT_EQUAL(size_t(5), hashing->size_visible());
  CPPUNIT_ASSERT(!downloads[1]->is_open());
  CPPUNIT_ASSERT(!downloads[1]->is_hash_failed());
  manager.download_list()->clear();
}

void
TestCommandEvents::test_hashing_pass_survives_hidden_additions_in_opened_handler() {
  core::Manager manager;
  auto hashing = *control->view_manager()->insert("hashing");
  control->view_manager()->insert("active");
  auto downloads = insert_downloads(1, missing_file, manager.download_list());
  manager.set_hashing_view(hashing);
  auto storage = [&] { return static_cast<const void*>(&*hashing->begin_visible()); };
  auto original_storage = storage();
  bool moved = false;
  opened_action = [&](core::Download* download) {
    if (download != downloads[0])
      return;

    for (int i = 0; storage() == original_storage && i < 4096; i++)
      hashing->set_not_visible(insert_downloads(1, missing_file, manager.download_list(), 100 + i, 1)[0]);

    moved = storage() != original_storage;
  };

  hashing->signal_changed().front()();

  CPPUNIT_ASSERT(moved);
  CPPUNIT_ASSERT_EQUAL(size_t(5), hashing->size_visible());
  CPPUNIT_ASSERT(downloads[0]->is_open());
  manager.download_list()->clear();
}

void
TestCommandEvents::test_check_hash_logs_after_erasing_and_failing_handler() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  rpc::call_command("d.hashing.set", int64_t{0}, rpc::make_target(downloads[0]));
  downloads[0]->bencode()->insert_key("libtorrent_resume", torrent::Object::create_map());
  torrent::log_open_output("test_events_log", [](const char* data, size_t size, int) { captured_log.append(data, size); });
  torrent::log_add_group_output(torrent::LOG_TORRENT_EVENTS, "test_events_log");
  queued_action = [](core::Download* download) {
    control->core()->download_list()->erase_ptr(download);
    throw torrent::storage_error("controlled hash queue failure");
  };

  control->core()->download_list()->check_hash(downloads[0]);

  CPPUNIT_ASSERT_MESSAGE(captured_log, captured_log.find("could not check hash : controlled hash queue failure") != std::string::npos);
  CPPUNIT_ASSERT_EQUAL(size_t(4), control->core()->download_list()->size());
}

void
TestCommandEvents::test_close_keeps_queued_hash_check() {
  auto hashing   = *control->view_manager()->insert("hashing");
  auto downloads = insert_downloads(1, missing_file, nullptr, 0, 1);
  auto active    = *control->view_manager()->insert("active");

  hashing->set_filter(torrent::Object("d.hashing="));

  rpc::call_command("d.open", torrent::Object(), rpc::make_target(downloads[0]));
  rpc::call_command("d.close", torrent::Object(), rpc::make_target(downloads[0]));
  hashing->filter();

  CPPUNIT_ASSERT(!downloads[0]->is_open());
  CPPUNIT_ASSERT_EQUAL(int64_t{1}, rpc::call_command_value("d.hashing", rpc::make_target(downloads[0])));
  CPPUNIT_ASSERT_EQUAL(size_t(1), hashing->size_visible());
  CPPUNIT_ASSERT_EQUAL(size_t(1), active->size_visible());
  CPPUNIT_ASSERT_EQUAL(1, closed_events);
  CPPUNIT_ASSERT_EQUAL(std::string(), handled);
}

void
TestCommandEvents::test_close_keeps_queued_hash_check_of_closed_download() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");

  rpc::call_command("d.close", torrent::Object(), rpc::make_target(downloads[0]));

  CPPUNIT_ASSERT_EQUAL(int64_t{1}, rpc::call_command_value("d.hashing", rpc::make_target(downloads[0])));
  CPPUNIT_ASSERT_EQUAL(0, closed_events);
  CPPUNIT_ASSERT_EQUAL(std::string(), handled);
}

void
TestCommandEvents::test_close_stops_running_hash_check() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  downloads[0]->download()->open();
  downloads[0]->download()->hash_check(false);
  CPPUNIT_ASSERT(downloads[0]->is_hash_checking());

  rpc::call_command("d.close", torrent::Object(), rpc::make_target(downloads[0]));

  CPPUNIT_ASSERT(!downloads[0]->is_open());
  CPPUNIT_ASSERT_EQUAL(int64_t{0}, rpc::call_command_value("d.hashing", rpc::make_target(downloads[0])));
}

void
TestCommandEvents::test_close_clears_stopped_hash_queue() {
  auto downloads = insert_downloads(0, missing_file);
  control->view_manager()->insert("active");

  rpc::call_command("d.close", torrent::Object(), rpc::make_target(downloads[0]));

  CPPUNIT_ASSERT_EQUAL(int64_t{0}, rpc::call_command_value("d.hashing", rpc::make_target(downloads[0])));
}

void
TestCommandEvents::test_close_clears_failed_hash_queue() {
  auto downloads = insert_downloads(1, missing_file);
  control->view_manager()->insert("active");
  downloads[0]->set_hash_failed(true);

  rpc::call_command("d.close", torrent::Object(), rpc::make_target(downloads[0]));

  CPPUNIT_ASSERT_EQUAL(int64_t{0}, rpc::call_command_value("d.hashing", rpc::make_target(downloads[0])));
}

void
TestCommandEvents::test_stop_removes_queued_hash_check() {
  auto downloads = insert_downloads(1, missing_file);
  auto started   = *control->view_manager()->insert("started");
  auto stopped   = insert_hidden_view("stopped", downloads);
  control->view_manager()->insert("active");
  started->set_event_removed(torrent::Object("scheduler.simple.removed="));
  stopped->set_event_added(torrent::Object("view.set_not_visible=started"));

  rpc::call_command("d.stop", torrent::Object(), rpc::make_target(downloads[0]));

  CPPUNIT_ASSERT_EQUAL(int64_t{0}, rpc::call_command_value("d.hashing", rpc::make_target(downloads[0])));
  CPPUNIT_ASSERT_EQUAL(std::string("d0"), handled);
}
