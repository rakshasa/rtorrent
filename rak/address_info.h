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

// Wrapper for addrinfo with focus on zero-copy conversion to and from
// the c-type and wrapper.
//
// Do use the wrapper on a pre-existing struct addrinfo, cast the
// pointer rather than the base type.

#ifndef RAK_ADDRESS_INFO_H
#define RAK_ADDRESS_INFO_H

#include <netdb.h>
#include <rak/socket_address.h>

namespace rak {

class address_info {
public:
  void                clear()                       { std::memset(this, 0, sizeof(address_info)); }

  int                 flags() const                 { return m_addrinfo.ai_flags; }
  void                set_flags(int f)              { m_addrinfo.ai_flags = f; }

  int                 family() const                { return m_addrinfo.ai_family; }
  void                set_family(int f)             { m_addrinfo.ai_family = f; }

  int                 socket_type() const           { return m_addrinfo.ai_socktype; }
  void                set_socket_type(int t)        { m_addrinfo.ai_socktype = t; }
  
  int                 protocol() const              { return m_addrinfo.ai_protocol; }
  void                set_protocol(int p)           { m_addrinfo.ai_protocol = p; }
  
  size_t              length() const                { return m_addrinfo.ai_addrlen; }

  socket_address*     address()                     { return reinterpret_cast<socket_address*>(m_addrinfo.ai_addr); }

  addrinfo*           c_addrinfo()                  { return &m_addrinfo; }
  const addrinfo*     c_addrinfo() const            { return &m_addrinfo; }

  address_info*       next()                        { return reinterpret_cast<address_info*>(m_addrinfo.ai_next); }

  static int          get_address_info(const char* node, int domain, int type, address_info** ai);

  static void         free_address_info(address_info* ai) { ::freeaddrinfo(ai->c_addrinfo()); }

  static const char*  strerror(int err)                   { return gai_strerror(err); }

private:
  addrinfo            m_addrinfo;
};

inline int
address_info::get_address_info(const char* node, int pfamily, int stype, address_info** ai) {
  address_info hints;
  hints.clear();
  hints.set_family(pfamily);
  hints.set_socket_type(stype);
  
  return ::getaddrinfo(node, NULL, hints.c_addrinfo(), reinterpret_cast<addrinfo**>(ai));
}

}

#endif
