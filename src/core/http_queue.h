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

#ifndef RTORRENT_CORE_HTTP_QUEUE_H
#define RTORRENT_CORE_HTTP_QUEUE_H

#include <list>
#include <iosfwd>
#include <tr1/functional>

namespace core {

class CurlGet;

class HttpQueue : private std::list<CurlGet*> {
public:
  typedef std::list<CurlGet*>                 base_type;
  typedef std::tr1::function<CurlGet* ()>     slot_factory;
  typedef std::tr1::function<void (CurlGet*)> slot_curl_get;
  typedef std::list<slot_curl_get>            signal_curl_get;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::empty;
  using base_type::size;

  HttpQueue() {}
  ~HttpQueue() { clear(); }

  // Note that any slots connected to the CurlGet signals must be
  // pushed in front of the erase slot added by HttpQueue::insert.
  //
  // Consider adding a flag to indicate whetever HttpQueue should
  // delete the stream.
  iterator    insert(const std::string& url, std::iostream* s);
  void        erase(iterator itr);

  void        clear();

  void             set_slot_factory(slot_factory s) { m_slot_factory = s; }

  signal_curl_get& signal_insert() { return m_signal_insert; }
  signal_curl_get& signal_erase()  { return m_signal_erase; }

private:
  slot_factory    m_slot_factory;
  signal_curl_get m_signal_insert;
  signal_curl_get m_signal_erase;
};

}

#endif
