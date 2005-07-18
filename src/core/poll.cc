// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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

#include <errno.h>
#include <unistd.h>
#include <stdexcept>
#include <sstream>
#include <ncurses.h>
#include <sigc++/bind.h>
#include <torrent/torrent.h>

#include "poll.h"
#include "curl_get.h"

namespace core {

void
Poll::poll(utils::Timer timeout) {
  // Do we want to clear m_maxFd in torrent::mark?
  //m_maxFd = 0;

  FD_ZERO(m_readSet);
  FD_ZERO(m_writeSet);
  FD_ZERO(m_exceptSet);

  FD_SET(0, m_readSet);

  torrent::mark(m_readSet, m_writeSet, m_exceptSet, &m_maxFd);

  if (m_curlStack.is_busy()) {
    int n = 0;

    m_curlStack.fdset(m_readSet, m_writeSet, m_exceptSet, &n);
    m_maxFd = std::max(m_maxFd, n);
  }

  timeval t = std::min(timeout, utils::Timer(torrent::get_next_timeout())).tval();

  if (m_maxFd >= FD_SETSIZE)
    throw std::runtime_error("Poll::work(): max fd >= FD_SETSIZE");

  errno = 0;
  m_maxFd = select(m_maxFd + 1, m_readSet, m_writeSet, m_exceptSet, &t);

  if (m_maxFd >= 0)
    return work();

  if (errno == EINTR) {
    m_slotSelectInterrupted();
    return work_input();
  }

  throw std::runtime_error("Poll::work(): select error");
}

void
Poll::work() {
  if (FD_ISSET(0, m_readSet))
    work_input();

  if (m_curlStack.is_busy())
    m_curlStack.perform();

  torrent::work(m_readSet, m_writeSet, m_exceptSet, m_maxFd);
}

void
Poll::work_input() {
  int key;

  while ((key = getch()) != ERR)
    m_slotReadStdin(key);
}  

Poll::SlotFactory
Poll::get_http_factory() {
  return sigc::bind(sigc::ptr_fun(&core::CurlGet::new_object), &m_curlStack);
}

}
