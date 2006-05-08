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

#ifndef RTORRENT_CORE_HASH_QUEUE_H
#define RTORRENT_CORE_HASH_QUEUE_H

#include <list>
#include <sigc++/slot.h>
#include <sigc++/connection.h>

namespace core {

class Download;
class DownloadList;

class HashQueueNode;

class HashQueue : private std::list<HashQueueNode*> {
public:
  typedef std::list<HashQueueNode*> Base;

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

  // Replace 'dl' with a slot.
  HashQueue(DownloadList* dl) : m_downloadList(dl) {}

  bool                is_queued(Download* download) const;

  void                insert(Download* d);

  // It's safe to try to remove downloads not in the queue. The hash
  // checking is not stopped if it has already started.
  void                remove(Download* d);

  iterator            find(Download* d);

private:
  void                receive_hash_done(Download* d);

  void                fill_queue();

  DownloadList*       m_downloadList;
};

class HashQueueNode {
public:
  HashQueueNode(Download* d) : m_download(d) {}
  ~HashQueueNode()                                       { disconnect(); }

  void                disconnect()                       { m_connection.disconnect(); }
  Download*           download()                         { return m_download; }

  void                set_connection(sigc::connection c) { m_connection = c; }

private:
  Download*           m_download;
  sigc::connection    m_connection;
};

}

#endif
