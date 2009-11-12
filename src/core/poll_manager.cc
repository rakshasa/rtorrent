// rTorrent - BitTorrent client
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

#include <stdexcept>
#include <rak/error_number.h>

#include "globals.h"
#include "control.h"
#include "manager.h"
#include "poll_manager.h"
#include "poll_manager_epoll.h"
#include "poll_manager_kqueue.h"
#include "poll_manager_select.h"

namespace core {

PollManager::PollManager(torrent::Poll* poll) :
  m_poll(poll) {

  if (m_poll == NULL)
    throw std::logic_error("PollManager::PollManager(...) received poll == NULL");
}

PollManager::~PollManager() {
  delete m_poll;
}

PollManager*
PollManager::create_poll_manager() {
  PollManager* pollManager = NULL;
  Log* log = &control->core()->get_log_important();

  const char* poll = getenv("RTORRENT_POLL");

  int maxOpen = sysconf(_SC_OPEN_MAX);

  if (poll != NULL) {
    if (!strcmp(poll, "epoll"))
      pollManager = PollManagerEPoll::create(maxOpen);
    else if (!strcmp(poll, "kqueue"))
      pollManager = PollManagerKQueue::create(maxOpen);
    else if (!strcmp(poll, "select"))
      pollManager = PollManagerSelect::create(maxOpen);

    if (pollManager == NULL)
      log->push_front(std::string("Cannot enable '") + poll + "' based polling.");
  }

  if (pollManager != NULL)
      log->push_front(std::string("Using '") + poll + "' based polling.");

  else if ((pollManager = PollManagerEPoll::create(maxOpen)) != NULL)
    log->push_front("Using 'epoll' based polling.");

  else if ((pollManager = PollManagerKQueue::create(maxOpen)) != NULL)
    log->push_front("Using 'kqueue' based polling.");

  else if ((pollManager = PollManagerSelect::create(maxOpen)) != NULL)
    log->push_front("Using 'select' based polling.");

  else
    throw std::runtime_error("Could not create any PollManager.");

  return pollManager;
}

void
PollManager::check_error() {
  if (rak::error_number::current().value() != rak::error_number::e_intr)
    throw std::runtime_error("Poll::work(): " + std::string(rak::error_number::current().c_str()));
}

}
