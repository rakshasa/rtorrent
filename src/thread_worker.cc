// rTorrent - BitTorrent library
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include "thread_worker.h"
#include "globals.h"
#include "control.h"

#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <rak/path.h>
#include <torrent/exceptions.h>

#include "core/manager.h"
#include "rpc/scgi.h"
#include "rpc/xmlrpc.h"
#include "rpc/parse_commands.h"

ThreadWorker::ThreadWorker() {
}

ThreadWorker::~ThreadWorker() {
  if (m_safe.scgi)
    m_safe.scgi->deactivate();
}

void
ThreadWorker::init_thread() {
  m_poll = core::create_poll();
  m_state = STATE_INITIALIZED;
}

bool
ThreadWorker::set_scgi(rpc::SCgi* scgi) {
  if (!__sync_bool_compare_and_swap(&m_safe.scgi, NULL, scgi))
    return false;

  change_xmlrpc_log();

  // The xmlrpc process call requires a global lock.
//   m_safe.scgi->set_slot_process(rak::mem_fn(&rpc::xmlrpc, &rpc::XmlRpc::process));

  // Synchronize in order to ensure the worker thread sees the updated
  // SCgi object.
  __sync_synchronize();
  queue_item((thread_base_func)&start_scgi);
  return true;
}

void
ThreadWorker::set_xmlrpc_log(const std::string& filename) {
  m_xmlrpcLog = filename;

  queue_item((thread_base_func)&msg_change_xmlrpc_log);
}

void
ThreadWorker::start_scgi(ThreadBase* baseThread) {
  ThreadWorker* thread = (ThreadWorker*)baseThread;

  if (thread->m_safe.scgi == NULL)
    throw torrent::internal_error("Tried to start SCGI but object was not present.");

  thread->m_safe.scgi->activate();
}

void
ThreadWorker::msg_change_xmlrpc_log(ThreadBase* baseThread) {
  ThreadWorker* thread = (ThreadWorker*)baseThread;

  acquire_global_lock();
  thread->change_xmlrpc_log();
  release_global_lock();
}

void
ThreadWorker::change_xmlrpc_log() {
  if (scgi() == NULL)
    return;

  if (scgi()->log_fd() != -1)
    ::close(scgi()->log_fd());

  if (m_xmlrpcLog.empty()) {
    control->core()->push_log("Closed XMLRPC log.");
    return;
  }

  scgi()->set_log_fd(open(rak::path_expand(m_xmlrpcLog).c_str(), O_WRONLY | O_APPEND | O_CREAT, 0644));

  if (scgi()->log_fd() == -1) {
    control->core()->push_log_std("Could not open XMLRPC log file '" + m_xmlrpcLog + "'.");
    return;
  }

  control->core()->push_log_std("Logging XMLRPC events to '" + m_xmlrpcLog + "'.");
}
