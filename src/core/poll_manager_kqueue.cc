// rTorrent - BitTorrent client
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

#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <sys/time.h>
#include <torrent/exceptions.h>
#include <torrent/poll_kqueue.h>
#include <torrent/torrent.h>

#include "poll_manager_kqueue.h"
#include "thread_base.h"

namespace core {

PollManagerKQueue*
PollManagerKQueue::create(int maxOpenSockets) {
  torrent::PollKQueue* p = torrent::PollKQueue::create(maxOpenSockets);

  if (p == NULL)
    return NULL;
  else
    return new PollManagerKQueue(p);
}

PollManagerKQueue::~PollManagerKQueue() {
}

void
PollManagerKQueue::poll(rak::timer timeout) {
  // Add 1ms to ensure we don't idle loop due to the lack of
  // resolution.
  torrent::perform();
  timeout = std::min(timeout, rak::timer(torrent::next_timeout())) + 1000;

  ThreadBase::release_global_lock();
  ThreadBase::entering_main_polling();

  int status = static_cast<torrent::PollKQueue*>(m_poll)->poll((timeout.usec() + 999) / 1000);

  ThreadBase::leaving_main_polling();
  ThreadBase::acquire_global_lock();

  if (status == -1)
    return check_error();

  torrent::perform();
  static_cast<torrent::PollKQueue*>(m_poll)->perform();
}

void
PollManagerKQueue::poll_simple(rak::timer timeout) {
  // Add 1ms to ensure we don't idle loop due to the lack of
  // resolution.
  timeout = std::min(timeout, rak::timer(torrent::next_timeout())) + 1000;

  if (static_cast<torrent::PollKQueue*>(m_poll)->poll((timeout.usec() + 999) / 1000) == -1)
    return check_error();

  static_cast<torrent::PollKQueue*>(m_poll)->perform();
}

}
