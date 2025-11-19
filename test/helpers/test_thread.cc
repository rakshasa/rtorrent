#include "config.h"

#include "test_thread.h"

#include <unistd.h>

#include "test/helpers/mock_function.h"
#include "torrent/exceptions.h"
#include "torrent/net/poll.h"

const int test_thread::test_flag_pre_stop;
const int test_thread::test_flag_long_timeout;

const int test_thread::test_flag_do_work;
const int test_thread::test_flag_pre_poke;
const int test_thread::test_flag_post_poke;

std::unique_ptr<test_thread>
test_thread::create() {
  // Needs to be called before Thread is created.
  mock_redirect_defaults();

  auto thread = new test_thread();
  return std::unique_ptr<test_thread>(thread);
}

test_thread::test_thread() :
  m_test_state(TEST_NONE),
  m_test_flags(0) {
}

test_thread::~test_thread() {
  if (is_active())
    stop_thread_wait();
}

void
test_thread::init_thread() {
  m_state = STATE_INITIALIZED;
  m_test_state = TEST_PRE_START;
}

void
test_thread::cleanup_thread() {
}

void
test_thread::call_events() {
  m_loop_count++;

  if ((m_test_flags & test_flag_pre_stop) && m_test_state == TEST_PRE_START && m_state == STATE_ACTIVE)
    m_test_state = TEST_PRE_STOP;

  if ((m_flags & flag_do_shutdown)) {
    if ((m_flags & flag_did_shutdown))
      throw torrent::internal_error("Already trigged shutdown.");

    m_flags |= flag_did_shutdown;
    throw torrent::shutdown_exception();
  }

  if ((m_test_flags & test_flag_pre_poke)) {
  }

  if ((m_test_flags & test_flag_do_work)) {
    usleep(10 * 1000); // TODO: Don't just sleep, as that give up core.
    m_test_flags &= ~test_flag_do_work;
  }

  if ((m_test_flags & test_flag_post_poke)) {
  }

  process_callbacks();
}

std::chrono::microseconds
test_thread::next_timeout() {
  if ((m_test_flags & test_flag_long_timeout))
    return std::chrono::microseconds(10s);
  else
    return std::chrono::microseconds(100ms);
}
