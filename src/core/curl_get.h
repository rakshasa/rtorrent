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

#ifndef RTORRENT_CORE_CURL_GET_H
#define RTORRENT_CORE_CURL_GET_H

#include <iosfwd>
#include <string>
#include <curl/curl.h>
#include <torrent/http.h>
#include <sigc++/signal.h>

struct CURLMsg;

namespace core {

class CurlGet : public torrent::Http {
 public:
  friend class CurlStack;

  CurlGet(CurlStack* s);
  virtual ~CurlGet();

  static CurlGet*    new_object(CurlStack* s);

  void               start();
  void               close();

  bool               is_busy() { return m_handle; }

  double             get_size_done();
  double             get_size_total();

 protected:
  CURL*              handle() { return m_handle; }

  void               perform(CURLMsg* msg);

 private:
  CurlGet(const CurlGet&);
  void operator = (const CurlGet&);

  friend size_t      curl_get_receive_write(void* data, size_t size, size_t nmemb, void* handle);

  CURL*              m_handle;

  CurlStack*         m_stack;
};

}

#endif
