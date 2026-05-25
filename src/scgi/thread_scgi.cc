#include "config.h"

#include "scgi/thread_scgi.h"

#include <fcntl.h>
#include <unistd.h>
#include <torrent/exceptions.h>
#include <torrent/utils/log.h>

#include "globals.h"
#include "rpc/scgi.h"

namespace scgi {

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
  if (m_scgi != nullptr)
    m_scgi.load()->stop();
}

rpc::SCgi*
ThreadScgi::scgi() {
  return m_scgi;
}

// TODO: Disable changing SCGI once set?

bool
ThreadScgi::set_scgi(rpc::SCgi* scgi) {
  rpc::SCgi* expected = nullptr;

  if (!m_scgi.compare_exchange_strong(expected, scgi))
    return false;

  change_rpc_log();

  callback([this]() {
      if (m_scgi == nullptr)
        throw torrent::internal_error("Tried to start SCGI but object was not present.");

      m_scgi.load()->activate();
    });

  return true;
}

void
ThreadScgi::set_rpc_log(const std::string& filename) {
  callback([this, filename]() {
      m_rpc_log_filename = filename;
      change_rpc_log();
    });
}

void
ThreadScgi::change_rpc_log() {
  if (scgi() == nullptr)
    return;

  if (scgi()->log_fd() != -1) {
    ::close(scgi()->log_fd());
    scgi()->set_log_fd(-1);

    lt_log_print(torrent::LOG_NOTICE, "Closed RPC log.", 0);
  }

  if (m_rpc_log_filename.empty())
    return;

  scgi()->set_log_fd(open(expand_path(m_rpc_log_filename).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644));

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

torrent::system::Thread* thread()                         { return scgi::ThreadScgi::thread_scgi(); }
std::thread::id          thread_id()                      { return scgi::ThreadScgi::thread_scgi()->thread_id(); }

void        callback_interrupt(torrent::system::callback_id& id, std::function<void ()>&& fn) { scgi::ThreadScgi::thread_scgi()->callback_interrupt(id, std::move(fn)); }

rpc::SCgi*  scgi()                                       { return scgi::ThreadScgi::thread_scgi()->scgi(); }
void        set_scgi(rpc::SCgi* scgi)                    { scgi::ThreadScgi::thread_scgi()->set_scgi(scgi); }
void        set_rpc_log(const std::string& filename)     { scgi::ThreadScgi::thread_scgi()->set_rpc_log(filename); }

} // namespace scgi_thread
