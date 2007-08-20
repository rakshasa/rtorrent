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

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <rak/socket_address.h>

#include <torrent/exceptions.h>
#include "socket_fd.h"

namespace utils {

inline void
SocketFd::check_valid() const {
  if (!is_valid())
    throw torrent::internal_error("SocketFd function called on an invalid fd.");
}

bool
SocketFd::set_nonblock() {
  check_valid();

  return fcntl(m_fd, F_SETFL, O_NONBLOCK) == 0;
}

bool
SocketFd::set_priority(priority_type p) {
  check_valid();
  int opt = p;

  return setsockopt(m_fd, IPPROTO_IP, IP_TOS, &opt, sizeof(opt)) == 0;
}

bool
SocketFd::set_reuse_address(bool state) {
  check_valid();
  int opt = state;

  return setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == 0;
}

bool
SocketFd::set_dont_route(bool state) {
  check_valid();
  int opt = state;

  return setsockopt(m_fd, SOL_SOCKET, SO_DONTROUTE, &opt, sizeof(opt)) == 0;
}

// bool
// SocketFd::set_bind_to_device(const char* device) {
//   check_valid();
//   struct ifreq ifr;
//   strlcpy(ifr.ifr_name, device, IFNAMSIZ);

//   return setsockopt(m_fd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof(ifr)) == 0;
// }

bool
SocketFd::set_send_buffer_size(uint32_t s) {
  check_valid();
  int opt = s;

  return setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt)) == 0;
}

bool
SocketFd::set_receive_buffer_size(uint32_t s) {
  check_valid();
  int opt = s;

  return setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt)) == 0;
}

int
SocketFd::get_error() const {
  check_valid();

  int err;
  socklen_t length = sizeof(err);

  if (getsockopt(m_fd, SOL_SOCKET, SO_ERROR, &err, &length) == -1)
    throw torrent::internal_error("SocketFd::get_error() could not get error");

  return err;
}

bool
SocketFd::open_stream() {
  return (m_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) != -1;
}

bool
SocketFd::open_datagram() {
  return (m_fd = socket(PF_INET, SOCK_DGRAM, 0)) != -1;
}

bool
SocketFd::open_local() {
  return (m_fd = socket(PF_LOCAL, SOCK_STREAM, 0)) != -1;
}

void
SocketFd::close() {
  if (::close(m_fd) && errno == EBADF)
    throw torrent::internal_error("SocketFd::close() called on an invalid file descriptor");
}

bool
SocketFd::bind(const rak::socket_address& sa) {
  check_valid();

  return !::bind(m_fd, sa.c_sockaddr(), sa.length());
}

bool
SocketFd::bind(const rak::socket_address& sa, unsigned int length) {
  check_valid();

  return !::bind(m_fd, sa.c_sockaddr(), length);
}

bool
SocketFd::connect(const rak::socket_address& sa) {
  check_valid();

  return !::connect(m_fd, sa.c_sockaddr(), sa.length()) || errno == EINPROGRESS;
}

bool
SocketFd::listen(int size) {
  check_valid();

  return !::listen(m_fd, size);
}

SocketFd
SocketFd::accept(rak::socket_address* sa) {
  check_valid();
  socklen_t len = sizeof(rak::socket_address);

  return SocketFd(::accept(m_fd, sa != NULL ? sa->c_sockaddr() : NULL, &len));
}

// unsigned int
// SocketFd::get_read_queue_size() const {
//   unsigned int v;

//   if (!is_valid() || ioctl(m_fd, SIOCINQ, &v) < 0)
//     throw internal_error("SocketFd::get_read_queue_size() could not be performed");

//   return v;
// }

// unsigned int
// SocketFd::get_write_queue_size() const {
//   unsigned int v;

//   if (!is_valid() || ioctl(m_fd, SIOCOUTQ, &v) < 0)
//     throw internal_error("SocketFd::get_write_queue_size() could not be performed");

//   return v;
// }

}
