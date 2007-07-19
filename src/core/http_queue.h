// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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
#include <sigc++/signal.h>

namespace core {

class CurlGet;

class HttpQueue : private std::list<CurlGet*> {
public:
  typedef std::list<CurlGet*>           Base;
  typedef sigc::signal1<void, CurlGet*> SignalHttp;
  typedef sigc::slot0<CurlGet*>         SlotFactory;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::empty;
  using Base::size;

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

  void        slot_factory(SlotFactory s) { m_slotFactory = s; }

  SignalHttp& signal_insert()             { return m_signalInsert; }
  SignalHttp& signal_erase()              { return m_signalErase; }

private:
  SlotFactory m_slotFactory;
  SignalHttp  m_signalInsert;
  SignalHttp  m_signalErase;
};

}

#endif
