#include "config.h"

#include "test/core/test_view.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "command_helpers.h"
#include "control.h"
#include "core/download.h"
#include "core/view.h"
#include "globals.h"
#include "rpc/parse_commands.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestView);

namespace {

const char* filter_command  = "test.view.filter";
const char* added_command   = "test.view.event_added";
const char* removed_command = "test.view.event_removed";

// View only stores these and hands them to the command layer as RPC targets,
// which casts the pointer without reading through it.
struct download_stub {
  alignas(core::Download) std::byte storage[sizeof(core::Download)];
};

std::deque<download_stub>    stub_storage;
std::vector<core::Download*> stub_downloads;

std::set<core::Download*>    matches_filter;
std::vector<core::Download*> added_events;
std::vector<core::Download*> removed_events;

std::shared_ptr<core::Download>
make_stub_download() {
  auto download = reinterpret_cast<core::Download*>(&stub_storage.emplace_back());

  stub_downloads.push_back(download);

  return std::shared_ptr<core::Download>(download, [](core::Download*) {});
}

// Renders a dispatch list as "d0,d2" so a failure names the downloads the
// handler saw, not just how many there were.
std::string
describe(const std::vector<core::Download*>& downloads) {
  std::string result;

  for (const auto& download : downloads) {
    auto itr = std::find(stub_downloads.begin(), stub_downloads.end(), download);

    if (!result.empty())
      result += ',';

    result += 'd' + std::to_string(std::distance(stub_downloads.begin(), itr));
  }

  return result;
}

torrent::Object
cmd_filter(core::Download* download, const torrent::Object&) {
  return torrent::Object(static_cast<int64_t>(matches_filter.count(download)));
}

torrent::Object
cmd_event_added(core::Download* download, const torrent::Object&) {
  added_events.push_back(download);
  return torrent::Object();
}

torrent::Object
cmd_event_removed(core::Download* download, const torrent::Object&) {
  removed_events.push_back(download);
  return torrent::Object();
}

torrent::Object
command_object(const char* command) {
  return torrent::Object(std::string(command) + "=");
}

} // namespace

void
TestView::setUp() {
  TestFixtureWithMainThread::setUp();

  if (control == nullptr)
    control = new Control;

  if (!rpc::commands.has(filter_command)) {
    CMD2_DL(filter_command, &cmd_filter);
    CMD2_DL(added_command, &cmd_event_added);
    CMD2_DL(removed_command, &cmd_event_removed);
  }

  stub_storage.clear();
  stub_downloads.clear();
  matches_filter.clear();
  added_events.clear();
  removed_events.clear();
}

void
TestView::tearDown() {
  TestFixtureWithMainThread::tearDown();
}

// d0 and d1 start visible and stop matching, d2 and d3 start filtered out and
// start matching. The two halves are the same size on purpose: a test that
// only counted the dispatches would pass even if both events were sent over
// the same half of the changed range.
void
TestView::test_filter_dispatches_events_to_the_right_downloads() {
  core::View view;
  view.initialize("test_view");

  auto d0 = make_stub_download();
  auto d1 = make_stub_download();
  auto d2 = make_stub_download();
  auto d3 = make_stub_download();

  view.insert(d0);
  view.insert(d1);
  view.insert(d2);
  view.insert(d3);

  view.set_visible(d0.get());
  view.set_visible(d1.get());

  CPPUNIT_ASSERT_EQUAL(core::View::size_type(2), view.size_visible());

  matches_filter = {d2.get(), d3.get()};

  view.set_filter(command_object(filter_command));
  view.set_event_added(command_object(added_command));
  view.set_event_removed(command_object(removed_command));

  added_events.clear();
  removed_events.clear();

  view.filter();

  CPPUNIT_ASSERT_EQUAL(std::string("d2,d3"), describe(added_events));
  CPPUNIT_ASSERT_EQUAL(std::string("d0,d1"), describe(removed_events));

  CPPUNIT_ASSERT_EQUAL(core::View::size_type(2), view.size_visible());
  CPPUNIT_ASSERT(*view.begin_visible() == d2);
  CPPUNIT_ASSERT(*(view.begin_visible() + 1) == d3);
}
