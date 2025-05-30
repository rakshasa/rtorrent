#include "config.h"

#include "test_main_thread.h"

#include <signal.h>

#include "test/helpers/mock_function.h"
#include "torrent/exceptions.h"
#include "torrent/net/resolver.h"
#include "torrent/utils/log.h"
#include "torrent/utils/scheduler.h"

std::unique_ptr<TestMainThread>
TestMainThread::create() {
  // Needs to be called before Thread is created.
  mock_redirect_defaults();
  return std::unique_ptr<TestMainThread>(new TestMainThread());
}

std::unique_ptr<TestMainThread>
TestMainThread::create_with_mock() {
  return std::unique_ptr<TestMainThread>(new TestMainThread());
}

void
TestMainThread::init_thread() {
  m_resolver = std::make_unique<torrent::net::Resolver>();
  m_state = STATE_INITIALIZED;

  //m_instrumentation_index = INSTRUMENTATION_POLLING_DO_POLL_MAIN - INSTRUMENTATION_POLLING_DO_POLL;

  init_thread_local();
}

void
TestMainThread::cleanup_thread() {
}

void
TestMainThread::call_events() {
  process_callbacks();
}

std::chrono::microseconds
TestMainThread::next_timeout() {
  return 10min;
}

void
TestFixtureWithMainThread::setUp() {
  test_fixture::setUp();

  m_main_thread = TestMainThread::create();
  m_main_thread->init_thread();
}

void
TestFixtureWithMainThread::tearDown() {
  m_main_thread.reset();

  test_fixture::tearDown();
}

void
TestFixtureWithMockAndMainThread::setUp() {
  test_fixture::setUp();

  m_main_thread = TestMainThread::create_with_mock();
  m_main_thread->init_thread();
}

void
TestFixtureWithMockAndMainThread::tearDown() {
  m_main_thread.reset();

  test_fixture::tearDown();
}

