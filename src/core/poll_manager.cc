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

#include <cerrno>
#include <stdexcept>

#include "poll_manager.h"

namespace core {

PollManager::PollManager(torrent::Poll* poll) :
  m_poll(poll) {

  if (m_poll == NULL)
    throw std::logic_error("PollManager::PollManager(...) received poll == NULL");

#if defined USE_VARIABLE_FDSET
  m_setSize = m_poll->max_open_sockets() / 8;
  m_readSet = (fd_set*)new char[m_setSize];
  m_writeSet = (fd_set*)new char[m_setSize];
  m_errorSet = (fd_set*)new char[m_setSize];
#else
  if (m_poll->max_open_sockets() > FD_SETSIZE)
    throw std::logic_error("PollManager::PollManager(...) received a max open sockets >= FD_SETSIZE, but USE_VARIABLE_FDSET was not defined");

  m_setSize = FD_SETSIZE / 8;
  m_readSet = new fd_set;
  m_writeSet = new fd_set;
  m_errorSet = new fd_set;
#endif
}

PollManager::~PollManager() {
  delete m_poll;
  delete m_readSet;
  delete m_writeSet;
  delete m_errorSet;
}

void
PollManager::check_error() {
  if (errno != EINTR)
    throw std::runtime_error("Poll::work(): select error");

  m_signalInterrupted.emit();
}

}
