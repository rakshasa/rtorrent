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

#ifndef RTORRENT_THREAD_WORKER_H
#define RTORRENT_THREAD_WORKER_H

#include "thread_base.h"

#include <rak/priority_queue_default.h>

namespace rpc {
class SCgi;
}

// Check if cacheline aligned with inheritance ends up taking two
// cachelines.

class lt_cacheline_aligned ThreadWorker : public ThreadBase {
public:
  ThreadWorker();
  ~ThreadWorker();

  const char*         name() const { return "rtorrent scgi"; }

  virtual void        init_thread();

  rpc::SCgi*          scgi() { return m_safe.scgi; }
  bool                set_scgi(rpc::SCgi* scgi);
  
  void                set_xmlrpc_log(const std::string& filename);

  static void         start_scgi(ThreadBase* thread);
  static void         msg_change_xmlrpc_log(ThreadBase* thread);

private:
  void                task_touch_log();

  void                change_xmlrpc_log();

  struct lt_cacheline_aligned safe_type {
    safe_type() : scgi(NULL) {}

    rpc::SCgi* scgi;
  };

  safe_type           m_safe;

  // The following types shall only be modified while holding the
  // global lock.
  std::string         m_xmlrpcLog;
};

#endif
