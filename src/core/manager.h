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

#ifndef RTORRENT_CORE_MANAGER_H
#define RTORRENT_CORE_MANAGER_H

#include <iosfwd>

#include "download_list.h"
#include "download_slot_map.h"
#include "download_store.h"
#include "hash_queue.h"
#include "http_queue.h"
#include "poll.h"
#include "log.h"

namespace torrent {
  class Bencode;
}

namespace core {

class Manager {
public:
  typedef DownloadList::iterator                    iterator;
  typedef sigc::slot1<void, DownloadList::iterator> SlotReady;
  typedef sigc::slot0<void>                         SlotFailed;

  Manager() : m_portFirst(6890), m_portLast(6999), m_debugTracker(-1) {}

  DownloadList&       get_download_list()                 { return m_downloadList; }
  DownloadStore&      get_download_store()                { return m_downloadStore; }
  HashQueue&          get_hash_queue()                    { return m_hashQueue; }
  HttpQueue&          get_http_queue()                    { return m_httpQueue; }

  DownloadSlotMap&    get_default_settings()              { return m_defaultSettings; }

  Poll&               get_poll()                          { return m_poll; }
  Log&                get_log_important()                 { return m_logImportant; }
  Log&                get_log_complete()                  { return m_logComplete; }

  void                initialize();
  void                cleanup();

  void                insert(std::string uri);
  iterator            erase(DownloadList::iterator itr);

  void                start(Download* d);
  void                stop(Download* d);

  const std::string&  get_dns()                               { return m_dns; }
  void                set_dns(const std::string& dns);

  void                set_port_range(int a, int b)            { m_portFirst = a; m_portLast = b; }
  void                set_listen_ip(const std::string& ip)    { m_listenIp = ip; }

  void                set_default_root(const std::string& path);

  void                debug_tracker()                         { m_debugTracker = 0; }

private:
  void                receive_http_failed(std::string msg);

  void                create_file(const std::string& uri);
  void                create_http(const std::string& uri);

  void                create_final(std::istream* s);

  void                setup_download(Download* itr);

  void                receive_debug_tracker(std::istream* s);

  DownloadList        m_downloadList;
  DownloadStore       m_downloadStore;
  HashQueue           m_hashQueue;
  HttpQueue           m_httpQueue;

  DownloadSlotMap     m_defaultSettings;

  Poll                m_poll;
  Log                 m_logImportant;
  Log                 m_logComplete;

  std::string         m_dns;
  int                 m_portFirst;
  int                 m_portLast;
  std::string         m_listenIp;

  std::string         m_defaultRoot;

  int                 m_debugTracker;
};

inline void
Manager::set_default_root(const std::string& path) {
  m_defaultRoot = path;

  if (!m_defaultRoot.empty() && *m_defaultRoot.rbegin() != '/')
    m_defaultRoot.push_back('/');
}

}

#endif
