// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

#include <torrent/connection_manager.h>
#include <torrent/poll.h>
#include <torrent/torrent.h>
#include <rak/socket_address.h>
#include <torrent/exceptions.h>

#include "utils/socket_fd.h"

#include "control.h"
#include "globals.h"
#include "scgi.h"

namespace rpc {

SCgi::~SCgi() {
  if (!get_fd().is_valid())
    return;

  control->poll()->remove_read(this);
  control->poll()->remove_error(this);
  control->poll()->close(this);

  get_fd().close();
  get_fd().clear();
}

bool
SCgi::open(uint16_t port) {
  if (!get_fd().open_stream())
    throw torrent::resource_error("Could not allocate socket for listening.");

  try {
    rak::socket_address sa;
    sa.sa_inet()->clear();
    sa.sa_inet()->set_port(port);

    if (!get_fd().set_nonblock() ||
        !get_fd().set_reuse_address(true) ||
        !get_fd().bind(sa) ||
        !get_fd().listen(10))
      throw torrent::resource_error("Could not allocate socket for listening.");

    torrent::connection_manager()->inc_socket_count();

    control->poll()->open(this);
    control->poll()->insert_read(this);
    control->poll()->insert_error(this);

  } catch (torrent::resource_error& e) {
    get_fd().close();
    get_fd().clear();

    throw e;
  }

  return true;
}

void
SCgi::event_read() {
  rak::socket_address sa;
  utils::SocketFd fd;

  while ((fd = get_fd().accept(&sa)).is_valid()) {
    SCgiTask* task = std::find_if(m_task, m_task + 10, std::mem_fun_ref(&SCgiTask::is_available));

    if (task == task + 10) {
      // Ergh... just closing for now.
      fd.close();
      continue;
    }
    
    task->open(this, fd.get_fd());
  }
}

void
SCgi::event_write() {
  throw torrent::internal_error("Listener does not support write().");
}

void
SCgi::event_error() {
  throw torrent::internal_error("SCGI listener port received an error event.");
}

bool
SCgi::receive_call(SCgiTask* task, const char* buffer, uint32_t length) {
  slot_write slotWrite;
  slotWrite.set(rak::mem_fn(task, &SCgiTask::receive_write));

  return m_slotProcess(buffer, length, slotWrite);
}

}
