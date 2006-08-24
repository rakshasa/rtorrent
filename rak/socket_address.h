// rak - Rakshasa's toolbox
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

// Wrappers for the various sockaddr types with focus on zero-copy
// casting between the original type and the wrapper class.
//
// The default ctor does not initialize any data.
//
// _n suffixes indicate that the argument or return value is in
// network byte order, _h that they are in hardware byte order.

// Add define for inet6 scope id?

#ifndef RAK_SOCKET_ADDRESS_H
#define RAK_SOCKET_ADDRESS_H

#include <cstring>
#include <string>
#include <stdexcept>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

namespace rak {

class socket_address_inet;
class socket_address_inet6;

class socket_address {
public:
  static const sa_family_t af_local  = AF_LOCAL;
  static const sa_family_t af_unix   = AF_UNIX;
  //  static const sa_family_t af_file   = AF_FILE;
  static const sa_family_t af_inet   = AF_INET;
  static const sa_family_t af_inet6  = AF_INET6;
  static const sa_family_t af_unspec = AF_UNSPEC;

  bool                is_valid() const;
  bool                is_bindable() const;
  bool                is_address_any() const;

  // Should we need to set AF_UNSPEC?
  void                clear()                                 { std::memset(this, 0, sizeof(socket_address)); set_family(); }

  sa_family_t         family() const                          { return m_sockaddr.sa_family; }
  void                set_family()                            { m_sockaddr.sa_family = af_unspec; }

  uint16_t            port() const;
  void                set_port(uint16_t p);

  std::string         address_str() const;
  bool                address_c_str(char* buf, socklen_t size) const;

  // Attemts to set it as an inet, then an inet6 address. It will
  // never set anything but net addresses, no local/unix.
  bool                set_address_str(const std::string& a)   { return set_address_c_str(a.c_str()); }
  bool                set_address_c_str(const char* a);

  uint32_t            length() const;

  socket_address_inet*        sa_inet()                       { return reinterpret_cast<socket_address_inet*>(this); }
  const socket_address_inet*  sa_inet() const                 { return reinterpret_cast<const socket_address_inet*>(this); }

  sockaddr*           c_sockaddr()                            { return &m_sockaddr; }
  sockaddr_in*        c_sockaddr_inet()                       { return &m_sockaddrInet; }

  const sockaddr*     c_sockaddr() const                      { return &m_sockaddr; }
  const sockaddr_in*  c_sockaddr_inet() const                 { return &m_sockaddrInet; }

#ifdef RAK_USE_INET6
  socket_address_inet6*       sa_inet6()                      { return reinterpret_cast<socket_address_inet6*>(this); }
  const socket_address_inet6* sa_inet6() const                { return reinterpret_cast<const socket_address_inet6*>(this); }

  sockaddr_in6*       c_sockaddr_inet6()                      { return &m_sockaddrInet6; }
  const sockaddr_in6* c_sockaddr_inet6() const                { return &m_sockaddrInet6; }
#endif

  // Copy a socket address which has the length 'length. Zero out any
  // extranous bytes and ensure it does not go beyond the size of this
  // struct.
  void                copy(const socket_address& src, size_t length);

  static socket_address*       cast_from(sockaddr* sa)        { return reinterpret_cast<socket_address*>(sa); }
  static const socket_address* cast_from(const sockaddr* sa)  { return reinterpret_cast<const socket_address*>(sa); }

  // The different families will be sorted according to the
  // sa_family_t's numeric value.
  bool                operator == (const socket_address& rhs) const;
  bool                operator  < (const socket_address& rhs) const;

  bool                operator == (const sockaddr& rhs) const { return *this == *cast_from(&rhs); }
  bool                operator == (const sockaddr* rhs) const { return *this == *cast_from(rhs); }
  bool                operator  < (const sockaddr& rhs) const { return *this == *cast_from(&rhs); }
  bool                operator  < (const sockaddr* rhs) const { return *this == *cast_from(rhs); }

private:
  union {
    sockaddr            m_sockaddr;
    sockaddr_in         m_sockaddrInet;
#ifdef RAK_USE_INET6
    sockaddr_in6        m_sockaddrInet6;
#endif
  };
};

// Remeber to set the AF_INET.

class socket_address_inet {
public:
  bool                is_any() const                          { return is_port_any() && is_address_any(); }
  bool                is_valid() const                        { return !is_port_any() && !is_address_any(); }
  bool                is_port_any() const                     { return port() == 0; }
  bool                is_address_any() const                  { return m_sockaddr.sin_addr.s_addr == htonl(INADDR_ANY); }

  void                clear()                                 { std::memset(this, 0, sizeof(socket_address_inet)); set_family(); }

  uint16_t            port() const                            { return ntohs(m_sockaddr.sin_port); }
  uint16_t            port_n() const                          { return m_sockaddr.sin_port; }
  void                set_port(uint16_t p)                    { m_sockaddr.sin_port = htons(p); }
  void                set_port_n(uint16_t p)                  { m_sockaddr.sin_port = p; }

  // Should address() return the uint32_t?
  in_addr             address() const                         { return m_sockaddr.sin_addr; }
  uint32_t            address_h() const                       { return ntohl(m_sockaddr.sin_addr.s_addr); }
  uint32_t            address_n() const                       { return m_sockaddr.sin_addr.s_addr; }
  std::string         address_str() const;
  bool                address_c_str(char* buf, socklen_t size) const;

  void                set_address(in_addr a)                  { m_sockaddr.sin_addr = a; }
  void                set_address_h(uint32_t a)               { m_sockaddr.sin_addr.s_addr = htonl(a); }
  void                set_address_n(uint32_t a)               { m_sockaddr.sin_addr.s_addr = a; }
  bool                set_address_str(const std::string& a)   { return set_address_c_str(a.c_str()); }
  bool                set_address_c_str(const char* a);

  void                set_address_any()                       { set_port(0); set_address_h(INADDR_ANY); }

  sa_family_t         family() const                          { return m_sockaddr.sin_family; }
  void                set_family()                            { m_sockaddr.sin_family = AF_INET; }

  sockaddr*           c_sockaddr()                            { return reinterpret_cast<sockaddr*>(&m_sockaddr); }
  sockaddr_in*        c_sockaddr_inet()                       { return &m_sockaddr; }

  const sockaddr*     c_sockaddr() const                      { return reinterpret_cast<const sockaddr*>(&m_sockaddr); }
  const sockaddr_in*  c_sockaddr_inet() const                 { return &m_sockaddr; }

  bool                operator == (const socket_address_inet& rhs) const;
  bool                operator < (const socket_address_inet& rhs) const;

private:
  struct sockaddr_in  m_sockaddr;
};

// Unique key for the address, excluding port numbers etc.
class socket_address_key {
public:
//   socket_address_host_key() {}

  socket_address_key(const socket_address& sa) {
    *this = sa;
  }

  socket_address_key& operator = (const socket_address& sa) {
    if (sa.family() == 0) {
      std::memset(this, 0, sizeof(socket_address_key));

    } else if (sa.family() == socket_address::af_inet) {
      // Using hardware order as we use operator < to compare when
      // using inet only.
      m_addr.s_addr = sa.sa_inet()->address_h();

    } else {
      // When we implement INET6 handling, embed the ipv4 address in
      // the ipv6 address.
      throw std::logic_error("socket_address_key(...) received an unsupported protocol family.");
    }

    return *this;
  }

//   socket_address_key& operator = (const socket_address_key& sa) {
//   }

  bool operator < (const socket_address_key& sa) const {
    // Compare the memory area instead.
    return m_addr.s_addr < sa.m_addr.s_addr;
  }    

private:
  union {
    in_addr m_addr;
// #ifdef RAK_USE_INET6
//     in_addr6 m_addr6;
// #endif
  };
};

inline bool
socket_address::is_valid() const {
  switch (family()) {
  case af_inet:
    return sa_inet()->is_valid();
//   case af_inet6:
//     return sa_inet6().is_valid();
  default:
    return false;
  }
}

inline bool
socket_address::is_bindable() const {
  switch (family()) {
  case af_inet:
    return !sa_inet()->is_address_any();
  default:
    return false;
  }
}

inline bool
socket_address::is_address_any() const {
  switch (family()) {
  case af_inet:
    return sa_inet()->is_address_any();
  default:
    return true;
  }
}

inline uint16_t
socket_address::port() const {
  switch (family()) {
  case af_inet:
    return sa_inet()->port();
  default:
    return 0;
  }
}

inline void
socket_address::set_port(uint16_t p) {
  switch (family()) {
  case af_inet:
    return sa_inet()->set_port(p);
  default:
    break;
  }
}

inline std::string
socket_address::address_str() const {
  switch (family()) {
  case af_inet:
    return sa_inet()->address_str();
  default:
    return std::string();
  }
}

inline bool
socket_address::address_c_str(char* buf, socklen_t size) const {
  switch (family()) {
  case af_inet:
    return sa_inet()->address_c_str(buf, size);
  default:
    return false;
  }
}

inline bool
socket_address::set_address_c_str(const char* a) {
  if (sa_inet()->set_address_c_str(a)) {
    sa_inet()->set_family();
    return true;

  } else {
    return false;
  }
}

// Is the zero length really needed, should we require some length?
inline uint32_t
socket_address::length() const {
  switch(family()) {
  case af_inet:
    return sizeof(sockaddr_in);
  default:
    return 0;
  }      
}

inline void
socket_address::copy(const socket_address& src, size_t length) {
  length = std::min(length, sizeof(socket_address));
  
  // Does this get properly optimized?
  std::memset(this, 0, sizeof(socket_address));
  std::memcpy(this, &src, length);
}

// Should we be able to compare af_unspec?

inline bool
socket_address::operator == (const socket_address& rhs) const {
  if (family() != rhs.family())
    return false;

  switch (family()) {
  case af_inet:
    return *sa_inet() == *rhs.sa_inet();
//   case af_inet6:
//     return *sa_inet6() == *rhs.sa_inet6();
  default:
    throw std::logic_error("socket_address::operator == (rhs) invalid type comparison.");
  }
}

inline bool
socket_address::operator < (const socket_address& rhs) const {
  if (family() != rhs.family())
    return family() < rhs.family();

  switch (family()) {
  case af_inet:
    return *sa_inet() < *rhs.sa_inet();
//   case af_inet6:
//     return *sa_inet6() < *rhs.sa_inet6();
  default:
    throw std::logic_error("socket_address::operator < (rhs) invalid type comparison.");
  }
}

inline std::string
socket_address_inet::address_str() const {
  char buf[INET_ADDRSTRLEN];

  if (!address_c_str(buf, INET_ADDRSTRLEN))
    return std::string();

  return std::string(buf);
}

inline bool
socket_address_inet::address_c_str(char* buf, socklen_t size) const {
  return inet_ntop(family(), &m_sockaddr.sin_addr, buf, size);
}

inline bool
socket_address_inet::set_address_c_str(const char* a) {
  return inet_pton(AF_INET, a, &m_sockaddr.sin_addr);
}

inline bool
socket_address_inet::operator == (const socket_address_inet& rhs) const {
  return
    m_sockaddr.sin_addr.s_addr == rhs.m_sockaddr.sin_addr.s_addr &&
    m_sockaddr.sin_port == rhs.m_sockaddr.sin_port;
}

inline bool
socket_address_inet::operator < (const socket_address_inet& rhs) const {
  return
    m_sockaddr.sin_addr.s_addr < rhs.m_sockaddr.sin_addr.s_addr ||
    (m_sockaddr.sin_addr.s_addr == rhs.m_sockaddr.sin_addr.s_addr &&
     m_sockaddr.sin_port < rhs.m_sockaddr.sin_port);
}

}

#endif
