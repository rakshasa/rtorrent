// libTorrent - BitTorrent library
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

#ifndef RTORRENT_UTILS_THREAD_BASE_H
#define RTORRENT_UTILS_THREAD_BASE_H

#include <pthread.h>
#include <sys/types.h>
#include <torrent/utils/thread_base.h>

#include "rak/priority_queue_default.h"
#include "core/poll_manager.h"

struct thread_queue_hack;

// Move this class to libtorrent.

struct thread_queue_hack;

class ThreadBase : public torrent::thread_base {
public:
  typedef rak::priority_queue_default priority_queue;
  typedef void (*thread_base_func)(ThreadBase*);
  typedef void* (*pthread_func)(void*);

  ThreadBase();
  virtual ~ThreadBase();

  priority_queue&     task_scheduler() { return m_taskScheduler; }

  virtual void        init_thread() = 0;

  void                start_thread();
  static void         stop_thread(ThreadBase* thread);

  // ATM, only interaction with a thread's allowed by other threads is
  // through the queue_item call.

  void                queue_item(thread_base_func newFunc);

  static void*        event_loop(ThreadBase* thread);

  // Only call this when global lock has been acquired, as it checks
  // ThreadBase::is_main_polling() which is only guaranteed to remain
  // 'false' if global lock keeps main thread from entering polling
  // again.
  //
  // Move to libtorrent some day.
  static void         interrupt_main_polling();

protected:
  inline rak::timer   client_next_timeout();

  void                call_queued_items();

  // TODO: Add thread name.

  // The timer needs to be sync'ed when updated...

  rak::priority_queue_default m_taskScheduler;

  rak::priority_item  m_taskShutdown;

  // Temporary hack to pass messages to a thread. This really needs to
  // be cleaned up and/or integrated into the priority queue itself.
  thread_queue_hack*  m_threadQueue;
};

#endif
