// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include <deque>
#include <string>

#include "rak/priority_queue_default.h"

namespace core {

class CurlGet;
class CurlSocket;

// By using a deque instead of vector we allow for cheaper removal of
// the oldest elements, those that will be first in the in the
// deque.
//
// This should fit well with the use-case of a http stack, thus
// we get most of the cache locality benefits of a vector with fast
// removal of elements.

class CurlStack : std::deque<CurlGet*> {
 public:
  friend class CurlGet;

  typedef std::deque<CurlGet*> base_type;

  using base_type::value_type;
  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::back;
  using base_type::front;

  using base_type::size;
  using base_type::empty;

  CurlStack();
  ~CurlStack();

  CurlGet*            new_object();
  CurlSocket*         new_socket(int fd);

  unsigned int        active() const                         { return m_active; }
  unsigned int        max_active() const                     { return m_maxActive; }
  void                set_max_active(unsigned int a)         { m_maxActive = a; }

  const std::string&  user_agent() const                     { return m_userAgent; }
  const std::string&  http_proxy() const                     { return m_httpProxy; }
  const std::string&  bind_address() const                   { return m_bindAddress; }
  const std::string&  http_capath() const                    { return m_httpCaPath; }
  const std::string&  http_cacert() const                    { return m_httpCaCert; }

  void                set_user_agent(const std::string& s)   { m_userAgent = s; }
  void                set_http_proxy(const std::string& s)   { m_httpProxy = s; }
  void                set_bind_address(const std::string& s) { m_bindAddress = s; }
  void                set_http_capath(const std::string& s)  { m_httpCaPath = s; }
  void                set_http_cacert(const std::string& s)  { m_httpCaCert = s; }

  bool                ssl_verify_host() const                { return m_ssl_verify_host; }
  bool                ssl_verify_peer() const                { return m_ssl_verify_peer; }
  void                set_ssl_verify_host(bool s)            { m_ssl_verify_host = s; }
  void                set_ssl_verify_peer(bool s)            { m_ssl_verify_peer = s; }

  long                dns_timeout() const                    { return m_dns_timeout; }
  void                set_dns_timeout(long timeout)          { m_dns_timeout = timeout; }

  static void         global_init();
  static void         global_cleanup();

  void                receive_action(CurlSocket* socket, int type);

  static int          set_timeout(void* handle, long timeout_ms, void* userp);

  void                transfer_done(void* handle, const char* msg);

 protected:
  void                add_get(CurlGet* get);
  void                remove_get(CurlGet* get);

 private:
  CurlStack(const CurlStack&);
  void operator = (const CurlStack&);

  void                receive_timeout();

  bool                process_done_handle();

  void*               m_handle;

  unsigned int        m_active;
  unsigned int        m_maxActive;

  rak::priority_item  m_taskTimeout;

  std::string         m_userAgent;
  std::string         m_httpProxy;
  std::string         m_bindAddress;
  std::string         m_httpCaPath;
  std::string         m_httpCaCert;

  long                m_ssl_verify_host;
  bool                m_ssl_verify_peer;
  long                m_dns_timeout;
};

}

#endif
