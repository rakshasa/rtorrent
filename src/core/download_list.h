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

  DownloadList() { }

  void                clear();

  void                session_save();

  iterator            find_hex(const char* hash);
  Download*           find_hex_ptr(const char* hash);

  // Might move this to DownloadFactory.
  Download*           create(std::istream* str, bool printLog);

  iterator            insert(Download* d);

  void                erase_ptr(Download* d);
  iterator            erase(iterator itr);

  //void                save(Download* d);

  bool                open(Download* d);
  void                open_throw(Download* d);

  void                close(Download* d);
  void                close_quick(Download* d);
  void                close_throw(Download* d);

  void                start_normal(Download* d);
  bool                start_try(Download* d);

  void                stop_normal(Download* d);
  bool                stop_try(Download* d);

  void                resume(Download* d);
  void                pause(Download* d);

  void                check_hash(Download* d);

  enum {
    SLOTS_INSERT,
    SLOTS_ERASE,
    SLOTS_OPEN,
    SLOTS_CLOSE,
    SLOTS_START,
    SLOTS_STOP,
    SLOTS_HASH_QUEUED,
    SLOTS_HASH_REMOVED,
    SLOTS_HASH_DONE,
    SLOTS_FINISHED,

    SLOTS_MAX_SIZE
  };

  slot_map&           slots(int m)                                      { return m_slotMaps[m]; }
  const slot_map&     slots(int m) const                                { return m_slotMaps[m]; }

  slot_map*           slot_map_begin()                                  { return m_slotMaps; }
  const slot_map*     slot_map_begin() const                            { return m_slotMaps; }
  slot_map*           slot_map_end()                                    { return m_slotMaps + SLOTS_MAX_SIZE; }
  const slot_map*     slot_map_end() const                              { return m_slotMaps + SLOTS_MAX_SIZE; }

  slot_map&           slot_map_insert()                                 { return m_slotMaps[SLOTS_INSERT]; }
  const slot_map&     slot_map_insert() const                           { return m_slotMaps[SLOTS_INSERT]; }
  slot_map&           slot_map_erase()                                  { return m_slotMaps[SLOTS_ERASE]; }
  const slot_map&     slot_map_erase() const                            { return m_slotMaps[SLOTS_ERASE]; }
  slot_map&           slot_map_open()                                   { return m_slotMaps[SLOTS_OPEN]; }
  const slot_map&     slot_map_open() const                             { return m_slotMaps[SLOTS_OPEN]; }
  slot_map&           slot_map_close()                                  { return m_slotMaps[SLOTS_CLOSE]; }
  const slot_map&     slot_map_close() const                            { return m_slotMaps[SLOTS_CLOSE]; }
  slot_map&           slot_map_start()                                  { return m_slotMaps[SLOTS_START]; }
  const slot_map&     slot_map_start() const                            { return m_slotMaps[SLOTS_START]; }
  slot_map&           slot_map_stop()                                   { return m_slotMaps[SLOTS_STOP]; }
  const slot_map&     slot_map_stop() const                             { return m_slotMaps[SLOTS_STOP]; }

  // The finished slots will be called when an active download with
  // "finished" == 0 performs a hash check which returns a done
  // torrent.
  //
  // But how to avoid sending 'completed' messages to the tracker?
  // Also we need to handle cases when a hashing torrent starts up
  // after a shutdown.

  slot_map&           slot_map_hash_queued()                            { return m_slotMaps[SLOTS_HASH_QUEUED]; }
  const slot_map&     slot_map_hash_queued() const                      { return m_slotMaps[SLOTS_HASH_QUEUED]; }
  slot_map&           slot_map_hash_removed()                           { return m_slotMaps[SLOTS_HASH_REMOVED]; }
  const slot_map&     slot_map_hash_removed() const                     { return m_slotMaps[SLOTS_HASH_REMOVED]; }
  slot_map&           slot_map_hash_done()                              { return m_slotMaps[SLOTS_HASH_DONE]; }
  const slot_map&     slot_map_hash_done() const                        { return m_slotMaps[SLOTS_HASH_DONE]; }
  slot_map&           slot_map_finished()                               { return m_slotMaps[SLOTS_FINISHED]; }
  const slot_map&     slot_map_finished() const                         { return m_slotMaps[SLOTS_FINISHED]; }

  bool                has_slot_insert(const std::string& key) const     { return slot_map_insert().find(key) != slot_map_insert().end(); }
  bool                has_slot_erase(const std::string& key) const      { return slot_map_erase().find(key) != slot_map_erase().end(); }
  bool                has_slot_open(const std::string& key) const       { return slot_map_open().find(key) != slot_map_open().end(); }
  bool                has_slot_close(const std::string& key) const      { return slot_map_close().find(key) != slot_map_close().end(); }
  bool                has_slot_start(const std::string& key) const      { return slot_map_start().find(key) != slot_map_start().end(); }
  bool                has_slot_stop(const std::string& key) const       { return slot_map_stop().find(key) != slot_map_stop().end(); }

  bool                has_slot_hash_queued(const std::string& key) const{ return slot_map_hash_queued().find(key) != slot_map_hash_queued().end(); }
  bool                has_slot_hash_done(const std::string& key) const  { return slot_map_hash_done().find(key) != slot_map_hash_done().end(); }
  bool                has_slot_finished(const std::string& key) const   { return slot_map_finished().find(key) != slot_map_finished().end(); }

  static void         erase_key(slot_map& sm, const std::string& key)   { sm.erase(key); }

private:
  DownloadList(const DownloadList&);
  void operator = (const DownloadList&);

  void                hash_done(Download* d);
  void                hash_queue(Download* d, int type);

  inline void         check_contains(Download* d);

  void                received_finished(Download* d);
  void                confirm_finished(Download* d);

  slot_map            m_slotMaps[SLOTS_MAX_SIZE];
};

}

#endif
