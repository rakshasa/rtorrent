#include "config.h"

#include "thread_session.h"

#include <torrent/exceptions.h>

#include "session/session_manager.h"

namespace session {

class ThreadSessionInternal {
public:
  static ThreadSession*      thread_session() { return ThreadSession::internal_thread_session(); }
  // static session::HttpStack* http_stack() { return ThreadSession::internal_thread_session()->http_stack(); }
};

ThreadSession* ThreadSession::m_thread_session{nullptr};

void
ThreadSession::create_thread() {
  auto thread = new ThreadSession;

  thread->m_manager = std::make_unique<SessionManager>(thread);

  m_thread_session          = thread;
  m_thread_session->m_state = STATE_INITIALIZED;
}

void
ThreadSession::destroy_thread() {
  delete m_thread_session;
  m_thread_session = nullptr;
}

ThreadSession*
ThreadSession::thread_session() {
  return m_thread_session;
}

// TODO: Remove '= 0'.
void
ThreadSession::init_thread() {
}

// TODO: Make sure we trigger session save before main thread exits, that it adds all required
// downloads to the queue.
void
ThreadSession::cleanup_thread() {
  m_manager->cleanup();
}

void
ThreadSession::call_events() {
  // lt_log_print_locked(torrent::LOG_THREAD_NOTICE, "Got thread_disk tick.");

  // TODO: Wait with shutdown until all session data is saved.

  process_callbacks();

  if ((m_flags & flag_do_shutdown)) {
    if (!m_manager->is_empty()) {
      // TODO: Figure out a better way to wait for session save to complete.
      // TODO: Sanity check to avoid getting stuck not shutting down.
      // TODO: Should we depend on next_timeout() instead of callbacks?
      return;
    }

    if ((m_flags & flag_did_shutdown))
      throw torrent::internal_error("Already trigged shutdown.");

    m_flags |= flag_did_shutdown;
    throw torrent::shutdown_exception();
  }
}

std::chrono::microseconds
ThreadSession::next_timeout() {
  // TODO: This leads to kqueue crash?
  // return std::chrono::microseconds(1h);
  return std::chrono::microseconds(10s);
}

} // namespace session

namespace session_thread {

torrent::utils::Thread* thread()                         { return session::ThreadSessionInternal::thread_session(); }
std::thread::id         thread_id()                      { return session::ThreadSessionInternal::thread_session()->thread_id(); }

void callback(void* target, std::function<void ()>&& fn) { session::ThreadSessionInternal::thread_session()->callback(target, std::move(fn)); }
void cancel_callback(void* target)                       { session::ThreadSessionInternal::thread_session()->cancel_callback(target); }
void cancel_callback_and_wait(void* target)              { session::ThreadSessionInternal::thread_session()->cancel_callback_and_wait(target); }

session::SessionManager* manager()                       { return session::ThreadSessionInternal::thread_session()->manager(); }

} // namespace session_thread
