#include "config.h"

#include "test/src/test_session_manager.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <dlfcn.h>
#include <future>
#include <mutex>
#include <sys/resource.h>
#include <unistd.h>
#include <vector>
#include <torrent/download.h>
#include <torrent/object.h>
#include <torrent/torrent.h>

#include "core/download.h"
#include "globals.h"
#include "session/download_storer.h"
#include "session/session_manager.h"
#include "session/thread_session.h"
#include "test/helpers/test_utils.h"

CPPUNIT_TEST_SUITE_REGISTRATION(TestSessionManager);

namespace {

std::atomic<bool> hold_renames{};
std::atomic<int>  held_renames{};

core::Download*
new_download(const std::string& name) {
  torrent::Object info = torrent::Object::create_map();
  info.insert_key("name", name);
  info.insert_key("length", int64_t{16});
  info.insert_key("piece length", int64_t{262144});
  info.insert_key("pieces", std::string(20, char(0)));

  auto object = new torrent::Object(torrent::Object::create_map());
  object->insert_key("info", info);
  object->insert_key("rtorrent", torrent::Object::create_map());
  object->insert_key("libtorrent_resume", torrent::Object::create_map());

  return new core::Download(torrent::download_add(object, 0));
}

void
wait_for_shutdown() {
  while (!session_thread::thread()->has_do_shutdown())
    usleep(1000);
}

} // namespace

extern "C" int
rename(const char* old_path, const char* new_path) noexcept(noexcept(::rename(old_path, new_path))) {
  if (hold_renames) {
    ++held_renames;

    while (hold_renames)
      usleep(1000);
  }

  static auto real_rename = reinterpret_cast<int (*)(const char*, const char*)>(dlsym(RTLD_NEXT, "rename"));
  if (real_rename == nullptr)
    std::abort();
  return real_rename(old_path, new_path);
}

void
TestSessionManager::setUp() {
  test_fixture::setUp();
  mock_redirect_defaults();

  rlimit limit{};
  if (getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_cur < 1024 && limit.rlim_max >= 1024) {
    limit.rlim_cur = 1024;
    setrlimit(RLIMIT_NOFILE, &limit);
  }

  torrent::initialize_main_thread();
  torrent::initialize();

  char session_dir[] = "/tmp/rtorrent_test_session_manager_XXXXXX";
  CPPUNIT_ASSERT(mkdtemp(session_dir) != nullptr);
  m_session_dir = session_dir;

  session::ThreadSession::create_thread();
  session_thread::manager()->set_path(m_session_dir);
  session_thread::thread()->start_thread();

  for (auto name : {"test_session_manager_0", "test_session_manager_1", "test_session_manager_2"})
    m_downloads.push_back(new_download(name));
}

void
TestSessionManager::tearDown() {
  hold_renames = false;
  held_renames = 0;

  session_thread::callback(wait_for_shutdown);
  session_thread::thread()->stop_thread_wait();
  session::ThreadSession::destroy_thread();

  for (auto download : m_downloads) {
    session::DownloadStorer(download).unlink_files(m_session_dir + "/");
    torrent::download_remove(*download->download());
    delete download;
  }

  m_downloads.clear();
  ::rmdir(m_session_dir.c_str());

  torrent::cleanup();

  test_fixture::tearDown();
}

void
TestSessionManager::test_save_waits_for_save_of_same_download() {
  auto manager = session_thread::manager();

  hold_renames = true;

  manager->save_full_download(m_downloads[0]);
  CPPUNIT_ASSERT(wait_for_true([] { return held_renames == 1; }));

  manager->save_resume_download(m_downloads[0]);
  manager->save_resume_download(m_downloads[1]);
  manager->save_resume_download(m_downloads[2]);
  manager->process_pending_builds(false);

  std::promise<void> processed;
  session_thread::callback([&processed] { processed.set_value(); });
  processed.get_future().wait();

  CPPUNIT_ASSERT_EQUAL(size_t{3}, manager->m_processing_save_counter.load());
  CPPUNIT_ASSERT_EQUAL(size_t{1}, manager->m_save_request_counter.load());

  hold_renames = false;

  CPPUNIT_ASSERT(wait_for_true([manager] { return manager->m_save_request_counter == 0; }));
}

void
TestSessionManager::test_flush_waits_for_save_of_same_download() {
  auto manager = session_thread::manager();

  hold_renames = true;

  manager->save_full_download(m_downloads[0]);
  CPPUNIT_ASSERT(wait_for_true([] { return held_renames == 1; }));

  session_thread::callback(wait_for_shutdown);
  manager->save_full_download(m_downloads[0]);

  std::unique_lock<std::mutex> lock(manager->m_mutex);

  auto                         queued = std::async(std::launch::async, [manager] {
      std::lock_guard<std::mutex> guard(manager->m_mutex);
      hold_renames = false;
      return manager->m_save_requests.size();
    });

  manager->flush_all_and_wait_unsafe(lock);

  CPPUNIT_ASSERT_EQUAL(size_t{1}, queued.get());
  CPPUNIT_ASSERT(manager->m_save_requests.empty());
}

bool
TestSessionManager::remove_waits_for_save_of(core::Download* saving) {
  auto                         manager  = session_thread::manager();
  bool                         removing = true;
  std::future<bool>            finished_while_removing;
  std::unique_lock<std::mutex> lock(manager->m_mutex);

  finished_while_removing = std::async(std::launch::async, [manager, &removing] {
      std::lock_guard<std::mutex> guard(manager->m_mutex);
      manager->m_processing_saves.clear();
      manager->m_finished_condition.notify_all();
      return removing;
    });

  manager->m_processing_saves["test"].second.download = saving;

  bool removed = manager->remove_completely_unsafe(m_downloads[0], lock);
  removing     = false;
  lock.unlock();

  CPPUNIT_ASSERT(!removed);
  return finished_while_removing.get();
}

void
TestSessionManager::test_remove_waits_for_matching_save() {
  CPPUNIT_ASSERT(remove_waits_for_save_of(m_downloads[0]));
}

void
TestSessionManager::test_remove_does_not_wait_for_other_save() {
  CPPUNIT_ASSERT(!remove_waits_for_save_of(m_downloads[1]));
}

void
TestSessionManager::test_remove_cancels_pending_build() {
  auto manager = session_thread::manager();

  manager->save_resume_download(m_downloads[0]);
  manager->save_resume_download(m_downloads[1]);
  manager->remove_download(m_downloads[0]);

  auto pending = manager->m_pending_builds;
  manager->process_pending_builds(false);
  CPPUNIT_ASSERT_EQUAL(size_t{1}, pending.size());
  CPPUNIT_ASSERT(pending.front() == m_downloads[1]);
}
