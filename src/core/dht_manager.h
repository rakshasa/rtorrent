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

#ifndef RTORRENT_CORE_DHT_MANAGER_H
#define RTORRENT_CORE_DHT_MANAGER_H

#include <rak/priority_queue_default.h>

#include <torrent/object.h>

namespace core {

class DhtManager {
public:
  DhtManager() : m_warned(false), m_start(dht_off) { }
  ~DhtManager();

  void                load_dht_cache();
  void                save_dht_cache();
  torrent::Object     dht_statistics();

  void                start_dht();
  void                stop_dht();
  void                auto_start()                 { if (m_start == dht_auto) start_dht(); }

  void                set_start(const std::string& arg);

private:
  static const int    dht_disable = 0;
  static const int    dht_off     = 1;
  static const int    dht_auto    = 2;
  static const int    dht_on      = 3;

  static const int    dht_settings_num = 4;
  static const char*  dht_settings[dht_settings_num];

  void                update();
  bool                log_statistics(bool force);

  unsigned int        m_dhtPrevCycle;
  unsigned int        m_dhtPrevQueriesSent;
  unsigned int        m_dhtPrevRepliesReceived;
  unsigned int        m_dhtPrevQueriesReceived;
  uint64_t            m_dhtPrevBytesUp;
  uint64_t            m_dhtPrevBytesDown;

  rak::priority_item  m_updateTimeout;
  rak::priority_item  m_stopTimeout;
  bool                m_warned;

  int                 m_start;
};

}

#endif
