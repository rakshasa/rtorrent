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

#ifndef RTORRENT_CORE_HASH_QUEUE_H
#define RTORRENT_CORE_HASH_QUEUE_H

#include <list>
#include <sigc++/slot.h>
#include <sigc++/connection.h>

namespace core {

class Download;

class HashQueueNode;

class HashQueue : private std::list<HashQueueNode*> {
public:
  typedef std::list<HashQueueNode*> Base;
  typedef sigc::slot0<void>         Slot;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::front;
  using Base::back;

  using Base::empty;
  using Base::size;

  // Should it be safe to try inserting already present/checked downloads?
  void insert(Download* d, Slot s);

  // It's safe to try to remove downloads not in the queue.
  void remove(Download* d);

private:
  void receive_hash_done(Base::iterator itr);

  void fill_queue();
};

class HashQueueNode {
public:
  HashQueueNode(Download* d, HashQueue::Slot s) : m_download(d), m_slot(s) {}
  ~HashQueueNode()                                    { m_connection.disconnect(); }

  Download*        get_download()                     { return m_download; }
  HashQueue::Slot  get_slot()                         { return m_slot; }

  void             set_connection(sigc::connection c) { m_connection = c; }

private:
  Download*        m_download;
  HashQueue::Slot  m_slot;
  sigc::connection m_connection;
};

}

#endif
