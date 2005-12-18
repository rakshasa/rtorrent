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

#ifndef RTORRENT_CORE_MANAGER_H
#define RTORRENT_CORE_MANAGER_H

#include <iosfwd>

#include "download_list.h"
#include "download_store.h"
#include "hash_queue.h"
#include "http_queue.h"
#include "poll_manager.h"
#include "log.h"

namespace torrent {
  class Bencode;
}

namespace core {

class Manager {
public:
  typedef DownloadList::iterator                    DListItr;
  typedef sigc::slot1<void, DownloadList::iterator> SlotReady;
  typedef sigc::slot0<void>                         SlotFailed;

  Manager();

  DownloadList&       get_download_list()                 { return m_downloadList; }
  DownloadStore&      get_download_store()                { return m_downloadStore; }
  HashQueue&          get_hash_queue()                    { return m_hashQueue; }
  HttpQueue&          get_http_queue()                    { return m_httpQueue; }

  PollManager*        get_poll_manager()                  { return m_pollManager; }
  Log&                get_log_important()                 { return m_logImportant; }
  Log&                get_log_complete()                  { return m_logComplete; }

  void                set_port_random(bool v)             { m_portRandom = v; }
  void                set_port_range(int a, int b)        { m_portFirst = a; m_portLast = b; }

  void                set_check_hash(bool state)          { m_checkHash = state; }

  // Really should find a more descriptive name.
  void                initialize_first();
  void                initialize_second();
  void                cleanup();

  void                listen_open();

  void                shutdown(bool force);

  DListItr            insert(std::istream* s);
  DListItr            erase(DListItr itr);

  void                start(Download* d);
  void                stop(Download* d);

  void                check_hash(Download* d);

  void                push_log(const std::string& msg)    { m_logImportant.push_front(msg); m_logComplete.push_front(msg); }

private:
  void                create_http(const std::string& uri);
  void                create_final(std::istream* s);

  void                initialize_bencode(Download* d);

  void                prepare_hash_check(Download* d);

  void                receive_http_failed(std::string msg);
  void                receive_download_done_hash_checked(Download* d);
  void                receive_download_inserted(Download* d);
  void                receive_download_done(Download* d);

  DownloadList        m_downloadList;
  DownloadStore       m_downloadStore;
  HashQueue           m_hashQueue;
  HttpQueue           m_httpQueue;

  PollManager*        m_pollManager;
  Log                 m_logImportant;
  Log                 m_logComplete;

  bool                m_portRandom;
  int                 m_portFirst;
  int                 m_portLast;

  bool                m_checkHash;
};

}

#endif
