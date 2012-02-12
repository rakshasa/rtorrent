// rTorrent - BitTorrent client
// Copyright (C) 2005-2008, Jari Sundell
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

#include <curl/curl.h>
#include <curl/multi.h>

#include <torrent/poll.h>
#include <torrent/exceptions.h>
#include <torrent/utils/thread_base.h>

#include "control.h"

#include "curl_socket.h"
#include "curl_stack.h"

namespace core {

int
CurlSocket::receive_socket(void* easy_handle, curl_socket_t fd, int what, void* userp, void* socketp) {
  CurlStack* stack = (CurlStack*)userp;
  CurlSocket* socket = (CurlSocket*)socketp;

  if (what == CURL_POLL_REMOVE) {
    // We also probably need the special code here as we're not
    // guaranteed that the fd will be closed, afaik.
    if (socket != NULL)
      socket->close();

    // TODO: Consider the possibility that we'll need to set the
    // fd-associated pointer curl holds to NULL.

    delete socket;
    return 0;
  }

  if (socket == NULL) {
    socket = stack->new_socket(fd);
    torrent::main_thread()->poll()->open(socket);

    // No interface for libcurl to signal when it's interested in error events.
    // Assume that hence it must always be interested in them.
    torrent::main_thread()->poll()->insert_error(socket);
  } 

  if (what == CURL_POLL_NONE || what == CURL_POLL_OUT)
    torrent::main_thread()->poll()->remove_read(socket);
  else
    torrent::main_thread()->poll()->insert_read(socket);

  if (what == CURL_POLL_NONE || what == CURL_POLL_IN)
    torrent::main_thread()->poll()->remove_write(socket);
  else
    torrent::main_thread()->poll()->insert_write(socket);

  return 0;
}

CurlSocket::~CurlSocket() {
  if (m_fileDesc != -1)
    throw torrent::internal_error("CurlSocket::~CurlSocket() m_fileDesc != -1.");
}

void
CurlSocket::close() {
  if (m_fileDesc == -1)
    throw torrent::internal_error("CurlSocket::close() m_fileDesc == -1.");

  torrent::main_thread()->poll()->closed(this);
  m_fileDesc = -1;
}

void
CurlSocket::event_read() {
#if (LIBCURL_VERSION_NUM >= 0x071003)
  return m_stack->receive_action(this, CURL_CSELECT_IN);
#else
  return m_stack->receive_action(this, 0);
#endif
}

void
CurlSocket::event_write() {
#if (LIBCURL_VERSION_NUM >= 0x071003)
  return m_stack->receive_action(this, CURL_CSELECT_OUT);
#else
  return m_stack->receive_action(this, 0);
#endif
}

void
CurlSocket::event_error() {
#if (LIBCURL_VERSION_NUM >= 0x071003)
  return m_stack->receive_action(this, CURL_CSELECT_ERR);
#else
  return m_stack->receive_action(this, 0);
#endif
}

}
