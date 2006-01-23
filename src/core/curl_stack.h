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

#ifndef RTORRENT_CORE_CURL_STACK_H
#define RTORRENT_CORE_CURL_STACK_H

#include <list>
#include <string>
#include <sigc++/slot.h>

namespace core {

class CurlGet;

class CurlStack {
 public:
  friend class CurlGet;

  typedef std::list<CurlGet*>   CurlGetList;
  typedef sigc::slot0<CurlGet*> SlotFactory;

  CurlStack();
  ~CurlStack();

  int                 get_size() const { return m_size; }
  bool                is_busy() const  { return !m_getList.empty(); }

  void                perform();

  // TODO: Set fd_set's only once?
  unsigned int        fdset(fd_set* readfds, fd_set* writefds, fd_set* exceptfds);

  SlotFactory         get_http_factory();

  const std::string&  user_agent() const                   { return m_userAgent; }
  void                set_user_agent(const std::string& s) { m_userAgent = s; }

  const std::string&  http_proxy() const                   { return m_httpProxy; }
  void                set_http_proxy(const std::string& s) { m_httpProxy = s; }

  const std::string&  bind_address() const                   { return m_bindAddress; }
  void                set_bind_address(const std::string& s) { m_bindAddress = s; }

  static void         global_init();
  static void         global_cleanup();

 protected:
  void                add_get(CurlGet* get);
  void                remove_get(CurlGet* get);

 private:
  CurlStack(const CurlStack&);
  void operator = (const CurlStack&);

  void*               m_handle;

  int                 m_size;
  CurlGetList         m_getList;

  std::string         m_userAgent;
  std::string         m_httpProxy;
  std::string         m_bindAddress;
};

}

#endif
