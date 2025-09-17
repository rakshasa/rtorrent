#ifndef RTORRENT_UTILS_SOCKET_FD_H
#define RTORRENT_UTILS_SOCKET_FD_H

#include <cinttypes>
#include <unistd.h>
#include <sys/socket.h>

namespace utils {

class SocketFd {
public:
  typedef uint8_t priority_type;

  SocketFd() : m_fd(-1) {}
  explicit SocketFd(int fd) : m_fd(fd) {}

  bool                is_valid() const                        { return m_fd >= 0; }

  int                 get_fd() const                          { return m_fd; }
  void                set_fd(int fd)                          { m_fd = fd; }

  bool                set_nonblock();
  bool                set_reuse_address(bool state);
  bool                set_dont_route(bool state);

  bool                set_bind_to_device(const char* device);

  bool                set_priority(priority_type p);

  bool                set_send_buffer_size(uint32_t s);
  bool                set_receive_buffer_size(uint32_t s);

  int                 get_error() const;

  bool                open_stream();
  bool                open_datagram();
  bool                open_local();
  void                close();

  void                clear()                                 { m_fd = -1; }

  bool                bind_sa(const sockaddr* sa, unsigned int length);

  bool                listen(int size);

private:
  inline void         check_valid() const;

  int                 m_fd;
  bool                m_ipv6_socket;
};

}

#endif
