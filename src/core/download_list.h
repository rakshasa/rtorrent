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
#include <map>
#include <string>
#include <sigc++/slot.h>

namespace core {

class Download;

// Container for all downloads. Add slots to the slot maps to cause
// some action to be taken when the torrent changes states. Don't
// change the states from outside of core.

class DownloadList : private std::list<Download*> {
public:
  typedef std::list<Download*>             base_type;
  typedef sigc::slot1<void, Download*>     slot_type;
  typedef std::map<std::string, slot_type> slot_map;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;
  using base_type::value_type;
  using base_type::pointer;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::empty;
  using base_type::size;

  ~DownloadList();

  void                session_save();

  // Might move this to DownloadFactory.
  Download*           create(std::istream* str, bool printLog);

  iterator            insert(Download* d);

  void                erase(Download* d);
  iterator            erase(iterator itr);

  //void                save(Download* d);

  void                open(Download* d);
  void                open_throw(Download* d);

  void                close(Download* d);
  void                close_throw(Download* d);

  void                start(Download* d);
  void                stop(Download* d);

  // These do not change the rtorrent:state.
  void                resume(Download* d);
  void                pause(Download* d);

  void                check_hash(Download* d);
  void                check_hash_throw(Download* d);

  void                hash_done(Download* d);

  slot_map&           slot_map_insert()                                 { return m_slotMapInsert; }
  slot_map&           slot_map_erase()                                  { return m_slotMapErase; }
  slot_map&           slot_map_open()                                   { return m_slotMapOpen; }
  slot_map&           slot_map_close()                                  { return m_slotMapClose; }
  slot_map&           slot_map_start()                                  { return m_slotMapStart; }
  slot_map&           slot_map_stop()                                   { return m_slotMapStop; }

  // The finished slots will be called when an active download with
  // "finished" == 0 performs a hash check which returns a done
  // torrent.
  //
  // But how to avoid sending 'completed' messages to the tracker?
  // Also we need to handle cases when a hashing torrent starts up
  // after a shutdown.

  slot_map&           slot_map_hash_done()                              { return m_slotMapHashDone; }
  slot_map&           slot_map_finished()                               { return m_slotMapFinished; }

  bool                has_slot_insert(const std::string& key) const     { return m_slotMapInsert.find(key) != m_slotMapInsert.end(); }
  bool                has_slot_erase(const std::string& key) const      { return m_slotMapErase.find(key) != m_slotMapErase.end(); }
  bool                has_slot_open(const std::string& key) const       { return m_slotMapOpen.find(key) != m_slotMapOpen.end(); }
  bool                has_slot_close(const std::string& key) const      { return m_slotMapClose.find(key) != m_slotMapClose.end(); }
  bool                has_slot_start(const std::string& key) const      { return m_slotMapStart.find(key) != m_slotMapStart.end(); }
  bool                has_slot_stop(const std::string& key) const       { return m_slotMapStop.find(key) != m_slotMapStop.end(); }

  bool                has_slot_hash_done(const std::string& key) const  { return m_slotMapFinished.find(key) != m_slotMapFinished.end(); }
  bool                has_slot_finished(const std::string& key) const   { return m_slotMapFinished.find(key) != m_slotMapFinished.end(); }

private:
  inline void         check_contains(Download* d);

  void                received_finished(Download* d);
  void                confirm_finished(Download* d);

  slot_map            m_slotMapInsert;
  slot_map            m_slotMapErase;
  slot_map            m_slotMapOpen;
  slot_map            m_slotMapClose;
  slot_map            m_slotMapStart;
  slot_map            m_slotMapStop;

  slot_map            m_slotMapHashDone;
  slot_map            m_slotMapFinished;
};

}

#endif
