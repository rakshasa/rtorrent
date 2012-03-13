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

#include <stdexcept>
#include <unistd.h>
#include <torrent/exceptions.h>
#include <torrent/poll_epoll.h>
#include <torrent/poll_kqueue.h>
#include <torrent/poll_select.h>

#include "globals.h"
#include "control.h"
#include "manager.h"
#include "poll_manager.h"

namespace core {

torrent::Poll*
create_poll() {
  const char* poll_name = getenv("RTORRENT_POLL");

  int maxOpen = sysconf(_SC_OPEN_MAX);

  torrent::Poll* poll = NULL;

  if (poll_name != NULL) {
    if (!strcmp(poll_name, "epoll"))
      poll = torrent::PollEPoll::create(maxOpen);
    else if (!strcmp(poll_name, "kqueue"))
      poll = torrent::PollKQueue::create(maxOpen);
    else if (!strcmp(poll_name, "select"))
      poll = torrent::PollSelect::create(maxOpen);

    if (poll == NULL)
      control->core()->push_log_std(std::string("Cannot enable '") + poll_name + "' based polling.");
  }

  if (poll != NULL)
    control->core()->push_log_std(std::string("Using '") + poll_name + "' based polling.");

  else if ((poll = torrent::PollEPoll::create(maxOpen)) != NULL)
    control->core()->push_log_std("Using 'epoll' based polling.");

  else if ((poll = torrent::PollKQueue::create(maxOpen)) != NULL)
    control->core()->push_log_std("Using 'kqueue' based polling.");

  else if ((poll = torrent::PollSelect::create(maxOpen)) != NULL)
    control->core()->push_log_std("Using 'select' based polling.");

  else
    throw torrent::internal_error("Could not create any Poll object.");

  return poll;
}

}
