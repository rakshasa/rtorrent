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

#include "config.h"

#include <fstream>
#include <sstream>
#include <torrent/object.h>
#include <torrent/dht_manager.h>
#include <torrent/object_stream.h>
#include <torrent/rate.h>
#include <torrent/utils/log.h>

#include "rpc/parse_commands.h"

#include "globals.h"

#include "control.h"
#include "dht_manager.h"
#include "download.h"
#include "download_store.h"
#include "manager.h"

namespace core {

const char* DhtManager::dht_settings[dht_settings_num] = { "disable", "off", "auto", "on" };

DhtManager::~DhtManager() {
  priority_queue_erase(&taskScheduler, &m_updateTimeout);
  priority_queue_erase(&taskScheduler, &m_stopTimeout);
}

void
DhtManager::load_dht_cache() {
  if (m_start == dht_disable || !control->core()->download_store()->is_enabled())
    return;

  torrent::Object cache = torrent::Object::create_map();
  std::fstream cache_file((control->core()->download_store()->path() + "rtorrent.dht_cache").c_str(), std::ios::in | std::ios::binary);

  if (cache_file.is_open()) {
    cache_file >> cache;

    // If the cache file is corrupted we will just discard it with an
    // error message.
    if (cache_file.fail()) {
      lt_log_print(torrent::LOG_DHT_WARN, "DHT cache file corrupted, discarding.");
      cache = torrent::Object::create_map();
    }
  }

  try {
    torrent::dht_manager()->initialize(cache);

    if (m_start == dht_on)
      start_dht();

  } catch (torrent::local_error& e) {
    lt_log_print(torrent::LOG_DHT_WARN, "DHT failed: %s", e.what());
  }
}

void
DhtManager::start_dht() {
  priority_queue_erase(&taskScheduler, &m_stopTimeout);

  if (!torrent::dht_manager()->is_valid() || torrent::dht_manager()->is_active())
    return;

  torrent::ThrottlePair throttles = control->core()->get_throttle(m_throttleName);
  torrent::dht_manager()->set_upload_throttle(throttles.first);
  torrent::dht_manager()->set_download_throttle(throttles.second);

  int port = rpc::call_command_value("dht.port");
  if (port <= 0)
    return;

  lt_log_print(torrent::LOG_DHT_INFO, "Starting DHT server on port %d.", port);

  try {
    torrent::dht_manager()->start(port);
    torrent::dht_manager()->reset_statistics();

    m_updateTimeout.slot() = std::bind(&DhtManager::update, this);
    priority_queue_insert(&taskScheduler, &m_updateTimeout, (cachedTime + rak::timer::from_seconds(60)).round_seconds());

    m_dhtPrevCycle = 0;
    m_dhtPrevQueriesSent = 0;
    m_dhtPrevRepliesReceived = 0;
    m_dhtPrevQueriesReceived = 0;
    m_dhtPrevBytesUp = 0;
    m_dhtPrevBytesDown = 0;

  } catch (torrent::local_error& e) {
    lt_log_print(torrent::LOG_DHT_ERROR, "DHT start failed: %s", e.what());
    m_start = dht_off;
  }
}

void
DhtManager::stop_dht() {
  priority_queue_erase(&taskScheduler, &m_updateTimeout);
  priority_queue_erase(&taskScheduler, &m_stopTimeout);

  if (torrent::dht_manager()->is_active()) {
    log_statistics(true);
    lt_log_print(torrent::LOG_DHT_INFO, "Stopping DHT server.");
    torrent::dht_manager()->stop();
  }
}

void
DhtManager::save_dht_cache() {
  if (!control->core()->download_store()->is_enabled() || !torrent::dht_manager()->is_valid())
    return;

  std::string filename = control->core()->download_store()->path() + "rtorrent.dht_cache";
  std::string filename_tmp = filename + ".new";
  std::fstream cache_file(filename_tmp.c_str(), std::ios::out | std::ios::trunc);

  if (!cache_file.is_open())
    return;

  torrent::Object cache = torrent::Object::create_map();
  cache_file << *torrent::dht_manager()->store_cache(&cache);

  if (!cache_file.good())
    return;

  cache_file.close();

  ::rename(filename_tmp.c_str(), filename.c_str());
}

void
DhtManager::set_mode(const std::string& arg) {
  int i;
  for (i = 0; i < dht_settings_num; i++) {
    if (arg == dht_settings[i]) {
      m_start = i;
      break;
    }
  }

  if (i == dht_settings_num)
    throw torrent::input_error("Invalid argument.");

  if (m_start == dht_off)
    stop_dht();
  else if (m_start == dht_on)
    start_dht();
}

void
DhtManager::update() {
  if (!torrent::dht_manager()->is_active())
    throw torrent::internal_error("DhtManager::update called with DHT inactive.");

  if (m_start == dht_auto && !m_stopTimeout.is_queued()) {
    DownloadList::const_iterator itr, end;

    for (itr = control->core()->download_list()->begin(), end = control->core()->download_list()->end(); itr != end; ++itr)
      if ((*itr)->download()->info()->is_active() && !(*itr)->download()->info()->is_private())
        break;
      
    if (itr == end) {
      m_stopTimeout.slot() = std::bind(&DhtManager::stop_dht, this);
      priority_queue_insert(&taskScheduler, &m_stopTimeout, (cachedTime + rak::timer::from_seconds(15 * 60)).round_seconds());
    }
  }

  // While bootstrapping (log_statistics returns true), check every minute if it completed, otherwise update every 15 minutes.
  if (log_statistics(false))
    priority_queue_insert(&taskScheduler, &m_updateTimeout, (cachedTime + rak::timer::from_seconds(60)).round_seconds());
  else
    priority_queue_insert(&taskScheduler, &m_updateTimeout, (cachedTime + rak::timer::from_seconds(15 * 60)).round_seconds());
}

bool
DhtManager::log_statistics(bool force) {
  torrent::DhtManager::statistics_type stats = torrent::dht_manager()->get_statistics();

  // Check for firewall problems.

  if (stats.cycle > 2 && stats.queries_sent - m_dhtPrevQueriesSent > 100 && stats.queries_received == m_dhtPrevQueriesReceived) {
    // We should have had clients ping us at least but have received
    // nothing, that means the UDP port is probably unreachable.
    if (torrent::dht_manager()->can_receive_queries())
      lt_log_print(torrent::LOG_DHT_WARN, "DHT port appears to be unreachable, no queries received.");

    torrent::dht_manager()->set_can_receive(false);
  }

  if (stats.queries_sent - m_dhtPrevQueriesSent > stats.num_nodes * 2 + 20 && stats.replies_received == m_dhtPrevRepliesReceived) {
    // No replies to over 20 queries plus two per node we have. Probably firewalled.
    if (!m_warned)
      lt_log_print(torrent::LOG_DHT_WARN, "DHT port appears to be firewalled, no replies received.");

    m_warned = true;
    return false;
  }

  m_warned = false;

  if (stats.queries_received > m_dhtPrevQueriesReceived)
    torrent::dht_manager()->set_can_receive(true);

  // Nothing to log while bootstrapping, but check again every minute.
  if (stats.cycle <= 1) {
    m_dhtPrevCycle = stats.cycle;
    return true;
  }

  // If bootstrap completed between now and the previous check, notify user.
  if (m_dhtPrevCycle == 1) {
    char buffer[128];
    snprintf(buffer, sizeof(buffer), "DHT bootstrap complete, have %d nodes in %d buckets.", stats.num_nodes, stats.num_buckets);
    control->core()->push_log_complete(buffer);
    m_dhtPrevCycle = stats.cycle;
    return false;
  };

  // Standard DHT statistics on first real cycle, and every 8th cycle
  // afterwards (i.e. every 2 hours), or when forced.
  if ((force && stats.cycle != m_dhtPrevCycle) || stats.cycle == 3 || stats.cycle > m_dhtPrevCycle + 7) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), 
             "DHT statistics: %d queries in, %d queries out, %d replies received, %lld bytes read, %lld bytes sent, "
             "%d known nodes in %d buckets, %d peers (highest: %d) tracked in %d torrents.",
             stats.queries_received - m_dhtPrevQueriesReceived,
             stats.queries_sent - m_dhtPrevQueriesSent,
             stats.replies_received - m_dhtPrevRepliesReceived,
             (long long unsigned int)(stats.down_rate.total() - m_dhtPrevBytesDown),
             (long long unsigned int)(stats.up_rate.total() - m_dhtPrevBytesUp),
             stats.num_nodes,
             stats.num_buckets,
             stats.num_peers,
             stats.max_peers,
             stats.num_trackers);

    control->core()->push_log_complete(buffer);

    m_dhtPrevCycle = stats.cycle;
    m_dhtPrevQueriesSent = stats.queries_sent;
    m_dhtPrevRepliesReceived = stats.replies_received;
    m_dhtPrevQueriesReceived = stats.queries_received;
    m_dhtPrevBytesUp = stats.up_rate.total();
    m_dhtPrevBytesDown = stats.down_rate.total();
  }

 return false;
}

torrent::Object
DhtManager::dht_statistics() {
  torrent::Object dhtStats = torrent::Object::create_map();

  dhtStats.insert_key("dht",              dht_settings[m_start]);
  dhtStats.insert_key("active",           torrent::dht_manager()->is_active());
  dhtStats.insert_key("throttle",         m_throttleName);

  if (torrent::dht_manager()->is_active()) {
    torrent::DhtManager::statistics_type stats = torrent::dht_manager()->get_statistics();

    dhtStats.insert_key("cycle",            stats.cycle);
    dhtStats.insert_key("queries_received", stats.queries_received);
    dhtStats.insert_key("queries_sent",     stats.queries_sent);
    dhtStats.insert_key("replies_received", stats.replies_received);
    dhtStats.insert_key("errors_received",  stats.errors_received);
    dhtStats.insert_key("errors_caught",    stats.errors_caught);
    dhtStats.insert_key("bytes_read",       stats.down_rate.total());
    dhtStats.insert_key("bytes_written",    stats.up_rate.total());
    dhtStats.insert_key("nodes",            stats.num_nodes);
    dhtStats.insert_key("buckets",          stats.num_buckets);
    dhtStats.insert_key("peers",            stats.num_peers);
    dhtStats.insert_key("peers_max",        stats.max_peers);
    dhtStats.insert_key("torrents",         stats.num_trackers);
  }

  return dhtStats;
}

void
DhtManager::set_throttle_name(const std::string& throttleName) {
  if (torrent::dht_manager()->is_active())
    throw torrent::input_error("Cannot set DHT throttle while active.");

  m_throttleName = throttleName;
}

}
