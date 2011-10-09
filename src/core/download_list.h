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

#ifndef RTORRENT_CORE_DOWNLOAD_LIST_H
#define RTORRENT_CORE_DOWNLOAD_LIST_H

#include <iosfwd>
#include <list>
#include <string>

namespace torrent {
  class HashString;
}

namespace core {

class Download;

// Container for all downloads. Add slots to the slot maps to cause
// some action to be taken when the torrent changes states. Don't
// change the states from outside of core.
//
// Fix apply_on_ratio if the base_type is changed.

class DownloadList : private std::list<Download*> {
public:
  typedef std::list<Download*>               base_type;

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

  iterator            find(const torrent::HashString& hash);

  iterator            find_hex(const char* hash);
  Download*           find_hex_ptr(const char* hash);

  // Might move this to DownloadFactory.
  Download*           create(std::istream* str, bool printLog);
  Download*           create(torrent::Object* obj, bool printLog);

  iterator            insert(Download* d);

  void                erase_ptr(Download* d);
  iterator            erase(iterator itr);

  //void                save(Download* d);

  bool                open(Download* d);
  void                open_throw(Download* d);

  void                close(Download* d);
  void                close_directly(Download* d);
  void                close_quick(Download* d);
  void                close_throw(Download* d);

  void                resume(Download* d, int flags = 0);
  void                pause(Download* d, int flags = 0);

  void                resume_default(Download* d) { resume(d); }
  void                pause_default(Download* d) { pause(d); }

  void                check_hash(Download* d);

  enum {
    D_SLOTS_INSERT,
    D_SLOTS_ERASE,
    D_SLOTS_OPEN,
    D_SLOTS_CLOSE,
    D_SLOTS_START,
    D_SLOTS_STOP,
    D_SLOTS_HASH_QUEUED,
    D_SLOTS_HASH_REMOVED,
    D_SLOTS_HASH_DONE,
    D_SLOTS_FINISHED,

    SLOTS_MAX_SIZE
  };

  static const char* slot_name(int m) {
    switch(m) {
    case D_SLOTS_INSERT: return "event.download.inserted";
    case D_SLOTS_ERASE: return "event.download.erased";
    case D_SLOTS_OPEN: return "event.download.opened";
    case D_SLOTS_CLOSE: return "event.download.closed";
    case D_SLOTS_START: return "event.download.resumed";
    case D_SLOTS_STOP: return "event.download.paused";
    case D_SLOTS_HASH_QUEUED: return "event.download.hash_queued";
    case D_SLOTS_HASH_REMOVED: return "event.download.hash_removed";
    case D_SLOTS_HASH_DONE: return "event.download.hash_done";
    case D_SLOTS_FINISHED: return "event.download.finished";
    default: return "BORK";
    }
  }

  // The finished slots will be called when an active download with
  // "finished" == 0 performs a hash check which returns a done
  // torrent.
  //
  // But how to avoid sending 'completed' messages to the tracker?
  // Also we need to handle cases when a hashing torrent starts up
  // after a shutdown.

private:
  DownloadList(const DownloadList&);
  void operator = (const DownloadList&);

  void                hash_done(Download* d);
  void                hash_queue(Download* d, int type);

  inline void         check_contains(Download* d);

  void                received_finished(Download* d);
  void                confirm_finished(Download* d);

  void                process_meta_download(Download* d);
};

}

#endif
