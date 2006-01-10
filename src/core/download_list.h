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

#ifndef RTORRENT_CORE_DOWNLOAD_LIST_H
#define RTORRENT_CORE_DOWNLOAD_LIST_H

#include <iosfwd>
#include <list>

#include "download_slot_map.h"

namespace core {

class Download;

// Container for all downloads. Add slots to the slot maps to cause
// some action to be taken when the torrent changes states. Don't
// change the states from outside of core.

class DownloadList : private std::list<Download*> {
public:
  typedef std::list<Download*> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;
  using Base::value_type;
  using Base::pointer;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::empty;
  using Base::size;

  ~DownloadList() { clear(); }

  iterator            insert(std::istream* str);
  iterator            erase(iterator itr);

  void                open(Download* d);
  void                close(Download* d);

  void                start(Download* d);
  void                stop(Download* d);

  DownloadSlotMap&    slot_map_insert()     { return m_slotMapInsert; }
  DownloadSlotMap&    slot_map_erase()      { return m_slotMapErase; }
  DownloadSlotMap&    slot_map_open()       { return m_slotMapOpen; }
  DownloadSlotMap&    slot_map_close()      { return m_slotMapClose; }
  DownloadSlotMap&    slot_map_start()      { return m_slotMapStart; }
  DownloadSlotMap&    slot_map_stop()       { return m_slotMapStop; }

  DownloadSlotMap&    slot_map_finished()   { return m_slotMapFinished; }

private:
  void                clear();

  void                finished(Download* d);

  DownloadSlotMap     m_slotMapInsert;
  DownloadSlotMap     m_slotMapErase;
  DownloadSlotMap     m_slotMapOpen;
  DownloadSlotMap     m_slotMapClose;
  DownloadSlotMap     m_slotMapStart;
  DownloadSlotMap     m_slotMapStop;

  DownloadSlotMap     m_slotMapFinished;
};

}

#endif
