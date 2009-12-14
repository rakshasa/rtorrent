// libTorrent - BitTorrent library
// Copyright (C) 2005-2007, Jari Sundell
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

#include "thread_base.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <torrent/exceptions.h>

#include "globals.h"

// A preliminary implementation of a global lock, to be moved
// somewhere more appropriate when it's put to use.

ThreadBase::global_lock_type ThreadBase::m_global = { 0, PTHREAD_MUTEX_INITIALIZER };

// Temporarly injected into config.h.
/* temp hack */
//#define __cacheline_aligned __attribute__((__aligned__(128)))

class __cacheline_aligned thread_queue_hack {
public:
  typedef ThreadBase::thread_base_func value_type;
  typedef ThreadBase::thread_base_func* iterator;

  static const unsigned int max_size = 32;

  thread_queue_hack() { std::memset(m_queue, 0, sizeof(thread_queue_hack)); }

  void     lock()   { while (!__sync_bool_compare_and_swap(&m_lock, 0, 1)) usleep(0); }
  void     unlock() { __sync_bool_compare_and_swap(&m_lock, 1, 0); }

  iterator begin() { return m_queue; }
  iterator max_capacity() { return m_queue + max_size; }

  iterator end_and_lock() { lock(); return std::find(begin(), max_capacity(), (value_type)NULL); }

  bool     empty() const { return m_queue[0] == NULL; }

  void push_back(value_type v) {
    iterator itr = end_and_lock();
  
    if (itr == max_capacity())
      throw torrent::internal_error("Overflowed thread_queue.");

    __sync_bool_compare_and_swap(itr, NULL, v);
    __sync_bool_compare_and_swap(&m_lock, 1, 0);
  }

  value_type* copy_and_clear(value_type* dest) {
    iterator itr = begin();
    lock();

    while (*itr != NULL) *dest++ = *itr++;

    clear_and_unlock();
    return dest;
  }

  void clear_and_unlock() {
    std::memset(m_queue, 0, sizeof(value_type) * (max_size + 1));
    m_lock = 0;
    __sync_synchronize();
  }

 private:
  int        m_lock;
  value_type m_queue[max_size + 1];
};

ThreadBase::ThreadBase() :
  m_state(STATE_UNKNOWN),
  m_pollManager(NULL) {
  // Init the poll manager in a special init function called by the
  // thread itself. Need to be careful with what external stuff
  // create_poll_manager calls in that case.

  m_threadQueue = new thread_queue_hack;
}

ThreadBase::~ThreadBase() {
  pthread_mutex_destroy(&ThreadBase::m_global.lock);
}

void
ThreadBase::start_thread() {
  if (m_state != STATE_INITIALIZED ||
      pthread_create(&m_thread, NULL, (pthread_func)&ThreadBase::event_loop, this))
    throw torrent::internal_error("Failed to create thread.");
}

void
ThreadBase::stop_thread() {
}

inline rak::timer
ThreadBase::client_next_timeout() {
  if (m_taskScheduler.empty())
    return rak::timer::from_seconds(10);
  else if (m_taskScheduler.top()->time() <= cachedTime)
    return 0;
  else
    return m_taskScheduler.top()->time() - cachedTime;
}

void*
ThreadBase::event_loop(ThreadBase* threadBase) {
  // Setup stuff...
  threadBase->m_state = STATE_ACTIVE;

  // Set local poll and priority queue.
  
  while (true) {
    // Check for new queued items set by other threads.
    if (!threadBase->m_threadQueue->empty())
      threadBase->call_queued_items();

    //     // Remember to add global lock thing to the main poll loop ++.

    rak::priority_queue_perform(&threadBase->m_taskScheduler, cachedTime);

    threadBase->m_pollManager->poll_simple(threadBase->client_next_timeout());
  }

  return NULL;
}

void
ThreadBase::call_queued_items() {
  thread_base_func result[thread_queue_hack::max_size];
  thread_base_func* first = result;
  thread_base_func* last = m_threadQueue->copy_and_clear((thread_base_func*)result);

  while (first != last && *first)
    (*first++)(this);
}

void
ThreadBase::queue_item(thread_base_func newFunc) {
  m_threadQueue->push_back(newFunc);
}
