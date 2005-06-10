// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_CORE_CURL_STACK_H
#define RTORRENT_CORE_CURL_STACK_H

#include <list>

namespace core {

class CurlGet;

class CurlStack {
 public:
  friend class CurlGet;

  typedef std::list<CurlGet*> CurlGetList;

  CurlStack();
  ~CurlStack();

  int         get_size() const { return m_size; }
  bool        is_busy() const  { return !m_getList.empty(); }

  void        perform();

  // TODO: Set fd_set's only once?
  void        fdset(fd_set* readfds, fd_set* writefds, fd_set* exceptfds, int* maxFd);

  static void init();
  static void cleanup();

 protected:
  void        add_get(CurlGet* get);
  void        remove_get(CurlGet* get);

 private:
  CurlStack(const CurlStack&);
  void operator = (const CurlStack&);

  void*       m_handle;

  int         m_size;
  CurlGetList m_getList;
};

}

#endif
