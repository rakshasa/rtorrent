#include "config.h"

#include "scgi/thread_scgi.h"

#include <fcntl.h>
#include <unistd.h>
#include <rak/path.h>
#include <torrent/exceptions.h>
#include <torrent/utils/log.h>

#include "rpc/scgi.h"

namespace scgi {

class ThreadScgiInternal {
public:
  static ThreadScgi* thread_scgi() { return ThreadScgi::internal_thread_scgi(); }
};

ThreadScgi* ThreadScgi::m_thread_scgi{};

void
ThreadScgi::create_thread() {
  auto thread = new ThreadScgi;

  m_thread_scgi          = thread;
  m_thread_scgi->m_state = STATE_INITIALIZED;
}

void
ThreadScgi::destroy_thread() {
  delete m_thread_scgi;
  m_thread_scgi = nullptr;
}

ThreadScgi*
ThreadScgi::thread_scgi() {
  return m_thread_scgi;
}

void
ThreadScgi::cleanup_thread() {
  if (m_scgi == nullptr)
    return;

  delete m_scgi.load();
  m_scgi = nullptr;
}

rpc::SCgi*
ThreadScgi::scgi() {
  return m_scgi;
}

bool
ThreadScgi::set_scgi(rpc::SCgi* scgi) {
  rpc::SCgi* expected = nullptr;

  if (!m_scgi.compare_exchange_strong(expected, scgi))
    return false;

  change_rpc_log();

  callback(nullptr, [this]() {
      if (m_scgi == NULL)
        throw torrent::internal_error("Tried to start SCGI but object was not present.");

      m_scgi.load()->activate();
    });

  return true;
}

void
ThreadScgi::set_rpc_log(const std::string& filename) {
  callback(nullptr, [this, filename]() {
      m_rpc_log_filename = filename;
      change_rpc_log();
    });
}

void
ThreadScgi::change_rpc_log() {
  if (scgi() == NULL)
    return;

  if (scgi()->log_fd() != -1) {
    ::close(scgi()->log_fd());
    scgi()->set_log_fd(-1);
    lt_log_print(torrent::LOG_NOTICE, "Closed RPC log.", 0);
  }

  if (m_rpc_log_filename.empty())
    return;

  scgi()->set_log_fd(open(rak::path_expand(m_rpc_log_filename).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644));

  if (scgi()->log_fd() == -1) {
    lt_log_print(torrent::LOG_NOTICE, "Could not open RPC log file '%s'.", m_rpc_log_filename.c_str());
    return;
  }

  lt_log_print(torrent::LOG_NOTICE, "Logging RPC events to '%s'.", m_rpc_log_filename.c_str());
}

void
ThreadScgi::call_events() {
  if ((m_flags & flag_do_shutdown)) {
    if ((m_flags & flag_did_shutdown))
      throw torrent::internal_error("Already trigged shutdown.");

    m_flags |= flag_did_shutdown;
    throw torrent::shutdown_exception();
  }

  process_callbacks();
}

std::chrono::microseconds
ThreadScgi::next_timeout() {
  // TODO: This leads to kqueue crash?
  // return std::chrono::microseconds(1h);
  return std::chrono::microseconds(10min);
}

} // namespace scgi

namespace scgi_thread {

torrent::utils::Thread* thread()                         { return scgi::ThreadScgiInternal::thread_scgi(); }
std::thread::id         thread_id()                      { return scgi::ThreadScgiInternal::thread_scgi()->thread_id(); }

void callback(void* target, std::function<void ()>&& fn) { scgi::ThreadScgiInternal::thread_scgi()->callback(target, std::move(fn)); }
void cancel_callback(void* target)                       { scgi::ThreadScgiInternal::thread_scgi()->cancel_callback(target); }
void cancel_callback_and_wait(void* target)              { scgi::ThreadScgiInternal::thread_scgi()->cancel_callback_and_wait(target); }

rpc::SCgi*  scgi()                                       { return scgi::ThreadScgiInternal::thread_scgi()->scgi(); }
void        set_scgi(rpc::SCgi* scgi)                    { scgi::ThreadScgiInternal::thread_scgi()->set_scgi(scgi); }
void        set_rpc_log(const std::string& filename)     { scgi::ThreadScgiInternal::thread_scgi()->set_rpc_log(filename); }

} // namespace scgi_thread
