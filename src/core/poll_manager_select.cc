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
#include <rak/allocators.h>
#include <torrent/exceptions.h>
#include <torrent/poll_select.h>
#include <torrent/torrent.h>

#include "poll_manager_select.h"
#include "thread_base.h"

namespace core {

PollManagerSelect::PollManagerSelect(torrent::Poll* p) : PollManager(p) {
#if defined USE_VARIABLE_FDSET
  m_setSize = (m_poll->open_max() + 7) / 8;
#else
#error Only variable fdset supported atm.
#endif
}

PollManagerSelect*
PollManagerSelect::create(int maxOpenSockets) {
  torrent::PollSelect* p = torrent::PollSelect::create(maxOpenSockets);

  if (p == NULL)
    return NULL;

  return new PollManagerSelect(p);
}

PollManagerSelect::~PollManagerSelect() {
}

void
PollManagerSelect::poll(rak::timer timeout) {
  static_cast<torrent::PollSelect*>(m_poll)->do_poll();
}

void
PollManagerSelect::poll_simple(rak::timer timeout) {
  static_cast<torrent::PollSelect*>(m_poll)->do_poll(torrent::PollSelect::poll_worker_thread);
}

}
