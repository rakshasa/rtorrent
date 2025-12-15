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

  m_thread_session = thread;
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

void
ThreadSession::init_thread() {
  m_state = STATE_INITIALIZED;
}

// TODO: Make sure we trigger session save before main thread exits, that it adds all required
// downloads to the queue.
void
ThreadSession::cleanup_thread() {
  // TODO: Do a final flush of all session data.
}

void
ThreadSession::call_events() {
  // lt_log_print_locked(torrent::LOG_THREAD_NOTICE, "Got thread_disk tick.");

  // TODO: Wait with shutdown until all session data is saved.

  if ((m_flags & flag_do_shutdown)) {
    if ((m_flags & flag_did_shutdown))
      throw torrent::internal_error("Already trigged shutdown.");

    m_flags |= flag_did_shutdown;
    throw torrent::shutdown_exception();
  }

  process_callbacks();
  // m_udns->flush();
  // process_callbacks();
}

std::chrono::microseconds
ThreadSession::next_timeout() {
  return std::chrono::microseconds(10s);
}

} // namespace session

namespace session_thread {

torrent::utils::Thread* thread()                         { return session::ThreadSessionInternal::thread_session(); }
std::thread::id         thread_id()                      { return session::ThreadSessionInternal::thread_session()->thread_id(); }

void callback(void* target, std::function<void ()>&& fn) { session::ThreadSessionInternal::thread_session()->callback(target, std::move(fn)); }
void cancel_callback(void* target)                       { session::ThreadSessionInternal::thread_session()->cancel_callback(target); }
void cancel_callback_and_wait(void* target)              { session::ThreadSessionInternal::thread_session()->cancel_callback_and_wait(target); }

// torrent::session::HttpStack* http_stack()                                    { return session::ThreadSessionInternal::http_stack(); }

} // namespace session_thread
