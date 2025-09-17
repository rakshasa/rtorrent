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
#include <torrent/exceptions.h>
#include <torrent/net/socket_address.h>

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

  if (m_ipv6_socket)
    return setsockopt(m_fd, IPPROTO_IPV6, IPV6_TCLASS, &opt, sizeof(opt)) == 0;
  else
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
  m_fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);

  if (m_fd == -1) {
    m_ipv6_socket = false;
    return (m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) != -1;
  }

  m_ipv6_socket = true;

  int zero = 0;
  return setsockopt(m_fd, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero)) != -1;
}

bool
SocketFd::open_datagram() {
  m_fd = socket(AF_INET6, SOCK_DGRAM, 0);
  if (m_fd == -1) {
    m_ipv6_socket = false;
    return (m_fd = socket(AF_INET, SOCK_DGRAM, 0)) != -1;
  }
  m_ipv6_socket = true;

  int zero = 0;
  return setsockopt(m_fd, IPPROTO_IPV6, IPV6_V6ONLY, &zero, sizeof(zero)) != -1;
}

bool
SocketFd::open_local() {
  return (m_fd = socket(AF_LOCAL, SOCK_STREAM, 0)) != -1;
}

void
SocketFd::close() {
  if (::close(m_fd) && errno == EBADF)
    throw torrent::internal_error("SocketFd::close() called on an invalid file descriptor");
}

bool
SocketFd::bind_sa(const sockaddr* sa, unsigned int length) {
  check_valid();

  if (m_ipv6_socket && sa->sa_family == AF_INET) {
    if (length < sizeof(sockaddr_in))
      throw torrent::input_error("SocketFd::bind_sa: invalid sockaddr length for AF_INET");

    auto mapped_sa = torrent::sin6_to_v4mapped_in(reinterpret_cast<const sockaddr_in*>(sa));
    return !::bind(m_fd, reinterpret_cast<const sockaddr*>(mapped_sa.get()), sizeof(sockaddr_in6));
  }

  return !::bind(m_fd, sa, length);
}

bool
SocketFd::listen(int size) {
  check_valid();

  return !::listen(m_fd, size);
}

}
