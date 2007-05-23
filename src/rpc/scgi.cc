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

#include <rak/error_number.h>
#include <rak/socket_address.h>
#include <sys/un.h>
#include <torrent/connection_manager.h>
#include <torrent/poll.h>
#include <torrent/torrent.h>
#include <torrent/exceptions.h>

#include "utils/socket_fd.h"

#include "control.h"
#include "globals.h"
#include "scgi.h"

namespace rpc {

SCgi::~SCgi() {
  if (!get_fd().is_valid())
    return;

  for (SCgiTask* itr = m_task, *last = m_task + max_tasks; itr != last; ++itr)
    if (itr->is_open())
      itr->close();

  control->poll()->remove_read(this);
  control->poll()->remove_error(this);
  control->poll()->close(this);

  get_fd().close();
  get_fd().clear();

  if (!m_path.empty())
    ::unlink(m_path.c_str());
}

void
SCgi::open_port(void* sa, unsigned int length, bool dontRoute) {
  if (!get_fd().open_stream() ||
      (dontRoute && !get_fd().set_dont_route(true)))
    throw torrent::resource_error("Could not open socket for listening: " + std::string(rak::error_number::current().c_str()));

  open(sa, length);
}

void
SCgi::open_named(const std::string& filename) {
  if (filename.empty() || filename.size() > 4096)
    throw torrent::resource_error("Invalid filename length.");

  char buffer[sizeof(sockaddr_un) + filename.size()];
  sockaddr_un* sa = reinterpret_cast<sockaddr_un*>(buffer);

  sa->sun_family = AF_LOCAL;
  std::memcpy(sa->sun_path, filename.c_str(), filename.size() + 1);

  if (!get_fd().open_local())
    throw torrent::resource_error("Could not open socket for listening.");

  open(sa, offsetof(struct sockaddr_un, sun_path) + filename.size() + 1);
  m_path = filename;
}

void
SCgi::open(void* sa, unsigned int length) {
  try {
    if (!get_fd().set_nonblock() ||
        !get_fd().set_reuse_address(true) ||
        !get_fd().bind(*reinterpret_cast<rak::socket_address*>(sa), length) ||
        !get_fd().listen(max_tasks))
      throw torrent::resource_error("Could not prepare socket for listening: " + std::string(rak::error_number::current().c_str()));

    torrent::connection_manager()->inc_socket_count();

    control->poll()->open(this);
    control->poll()->insert_read(this);
    control->poll()->insert_error(this);

  } catch (torrent::resource_error& e) {
    get_fd().close();
    get_fd().clear();

    throw e;
  }
}

void
SCgi::event_read() {
  rak::socket_address sa;
  utils::SocketFd fd;

  while ((fd = get_fd().accept(&sa)).is_valid()) {
    SCgiTask* task = std::find_if(m_task, m_task + max_tasks, std::mem_fun_ref(&SCgiTask::is_available));

    if (task == task + max_tasks) {
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
