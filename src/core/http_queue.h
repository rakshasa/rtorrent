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
  iterator    insert(const std::string& url);
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
