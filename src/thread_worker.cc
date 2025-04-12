#include "config.h"

#include "thread_worker.h"
#include "globals.h"
#include "control.h"

#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <rak/path.h>
#include <torrent/exceptions.h>
#include <torrent/poll.h>

#include "core/manager.h"
#include "rpc/scgi.h"
#include "rpc/parse_commands.h"

ThreadWorker::~ThreadWorker() {
  if (m_scgi)
    m_scgi.load()->deactivate();
}

void
ThreadWorker::init_thread() {
  m_poll = std::unique_ptr<torrent::Poll>(core::create_poll());
  m_state = STATE_INITIALIZED;
}

bool
ThreadWorker::set_scgi(rpc::SCgi* scgi) {
  rpc::SCgi* expected = nullptr;
  if (!m_scgi.compare_exchange_strong(expected, scgi))
    return false;

  change_rpc_log();

  callback(nullptr, [this]() {
      start_scgi(this);
    });

  return true;
}

void
ThreadWorker::set_rpc_log(const std::string& filename) {
  m_rpcLog = filename;

  callback(nullptr, [this]() {
      msg_change_rpc_log(this);
    });
}

void
ThreadWorker::start_scgi(ThreadBase* baseThread) {
  ThreadWorker* thread = (ThreadWorker*)baseThread;

  if (thread->scgi() == NULL)
    throw torrent::internal_error("Tried to start SCGI but object was not present.");

  thread->scgi()->activate();
}

void
ThreadWorker::msg_change_rpc_log(ThreadBase* baseThread) {
  ThreadWorker* thread = (ThreadWorker*)baseThread;

  acquire_global_lock();
  thread->change_rpc_log();
  release_global_lock();
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

  if (m_rpcLog.empty())
    return;

  scgi()->set_log_fd(open(rak::path_expand(m_rpcLog).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644));

  if (scgi()->log_fd() == -1) {
    control->core()->push_log_std("Could not open RPC log file '" + m_rpcLog + "'.");
    return;
  }

  control->core()->push_log_std("Logging RPC events to '" + m_rpcLog + "'.");
}
