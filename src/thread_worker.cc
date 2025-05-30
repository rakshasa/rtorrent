#include "config.h"

#include "thread_worker.h"

#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <rak/path.h>
#include <torrent/exceptions.h>

#include "globals.h"
#include "control.h"
#include "core/manager.h"
#include "rpc/scgi.h"
#include "rpc/parse_commands.h"

ThreadWorker::~ThreadWorker() = default;

void
ThreadWorker::init_thread() {
  m_state = STATE_INITIALIZED;
}

void
ThreadWorker::cleanup_thread() {
  if (m_scgi != nullptr)
    m_scgi.load()->deactivate();
}

bool
ThreadWorker::set_scgi(rpc::SCgi* scgi) {
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
ThreadWorker::set_rpc_log(const std::string& filename) {
  callback(nullptr, [this, filename]() {
      m_rpc_log_filename = filename;
      change_rpc_log();
    });
}

void
ThreadWorker::change_rpc_log() {
  if (scgi() == NULL)
    return;

  if (scgi()->log_fd() != -1) {
    ::close(scgi()->log_fd());
    scgi()->set_log_fd(-1);
    control->core()->push_log("Closed RPC log.");
  }

  if (m_rpc_log_filename.empty())
    return;

  scgi()->set_log_fd(open(rak::path_expand(m_rpc_log_filename).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644));

  if (scgi()->log_fd() == -1) {
    control->core()->push_log_std("Could not open RPC log file '" + m_rpc_log_filename + "'.");
    return;
  }

  control->core()->push_log_std("Logging RPC events to '" + m_rpc_log_filename + "'.");
}

void
ThreadWorker::call_events() {
  if ((m_flags & flag_do_shutdown)) {
    if ((m_flags & flag_did_shutdown))
      throw torrent::internal_error("Already trigged shutdown.");

    m_flags |= flag_did_shutdown;
    throw torrent::shutdown_exception();
  }

  process_callbacks();
}

std::chrono::microseconds
ThreadWorker::next_timeout() {
  return std::chrono::microseconds(10min);
}
