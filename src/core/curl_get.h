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

#ifndef RTORRENT_CORE_CURL_GET_H
#define RTORRENT_CORE_CURL_GET_H

#include <iosfwd>
#include <string>
#include <curl/curl.h>
#include <torrent/http.h>

#include "rak/priority_queue_default.h"

namespace core {

class CurlStack;

class CurlGet : public torrent::Http {
public:
  friend class CurlStack;

  CurlGet(CurlStack* s) : m_active(false), m_handle(NULL), m_stack(s) {}
  virtual ~CurlGet();

  void               start();
  void               close();

  bool               is_busy() const    { return m_handle; }
  bool               is_active() const  { return m_active; }

  void               set_active(bool a) { m_active = a; }

  double             size_done();
  double             size_total();

  CURL*              handle()           { return m_handle; }

private:
  CurlGet(const CurlGet&);
  void operator = (const CurlGet&);

  void               receive_timeout();

  bool               m_active;

  rak::priority_item m_taskTimeout;
  
  CURL*              m_handle;
  CurlStack*         m_stack;
};

}

#endif
