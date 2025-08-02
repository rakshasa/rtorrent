#include "config.h"

#include <fstream>
#include <sstream>
#include <torrent/object.h>
#include <torrent/object_stream.h>
#include <torrent/rate.h>
#include <torrent/tracker/dht_controller.h>
#include <torrent/utils/log.h>

#include "rpc/parse_commands.h"

#include "globals.h"

#include "control.h"
#include "dht_manager.h"
#include "download.h"
#include "download_store.h"
#include "manager.h"

#define LT_LOG_THIS(log_fmt, ...)                                       \
  lt_log_print_subsystem(torrent::LOG_DHT_MANAGER, "dht_manager", log_fmt, __VA_ARGS__);

namespace core {

const char* DhtManager::dht_settings[dht_settings_num] = { "disable", "off", "auto", "on" };

DhtManager::~DhtManager() {
  torrent::this_thread::scheduler()->erase(&m_update_timeout);
  torrent::this_thread::scheduler()->erase(&m_stop_timeout);
}

void
DhtManager::load_dht_cache() {
  if (m_start == dht_disable || !control->core()->download_store()->is_enabled()) {
    LT_LOG_THIS("ignoring cache file", 0);
    return;
  }

  std::string cache_filename = control->core()->download_store()->path() + "rtorrent.dht_cache";
  std::fstream cache_stream(cache_filename.c_str(), std::ios::in | std::ios::binary);

  torrent::Object cache = torrent::Object::create_map();

  if (cache_stream.is_open()) {
    cache_stream >> cache;

    // If the cache file is corrupted we will just discard it with an
    // error message.
    if (cache_stream.fail()) {
      LT_LOG_THIS("cache file corrupted, discarding (path:%s)", cache_filename.c_str());
      cache = torrent::Object::create_map();
    } else {
      LT_LOG_THIS("cache file read (path:%s)", cache_filename.c_str());
    }

  } else {
    LT_LOG_THIS("could not open cache file (path:%s)", cache_filename.c_str());
  }

  torrent::dht_controller()->initialize(cache);

  if (m_start == dht_on)
    start_dht();
}

void
DhtManager::start_dht() {
  torrent::this_thread::scheduler()->erase(&m_stop_timeout);

  if (!torrent::dht_controller()->is_valid()) {
    LT_LOG_THIS("server start skipped, manager is uninitialized", 0);
    return;
  }

  if (torrent::dht_controller()->is_active()) {
    LT_LOG_THIS("server start skipped, already active", 0);
    return;
  }

  torrent::ThrottlePair throttles = control->core()->get_throttle(m_throttleName);
  torrent::dht_controller()->set_upload_throttle(throttles.first);
  torrent::dht_controller()->set_download_throttle(throttles.second);

  int port = rpc::call_command_value("dht.port");

  if (port <= 0)
    return;

  if (!torrent::dht_controller()->start(port)) {
    m_start = dht_off;
    return;
  }

  torrent::dht_controller()->reset_statistics();

  m_update_timeout.slot() = [this] { update(); };

  torrent::this_thread::scheduler()->wait_for_ceil_seconds(&m_update_timeout, 60s);

  m_dhtPrevCycle = 0;
  m_dhtPrevQueriesSent = 0;
  m_dhtPrevRepliesReceived = 0;
  m_dhtPrevQueriesReceived = 0;
  m_dhtPrevBytesUp = 0;
  m_dhtPrevBytesDown = 0;
}

void
DhtManager::stop_dht() {
  torrent::this_thread::scheduler()->erase(&m_update_timeout);
  torrent::this_thread::scheduler()->erase(&m_stop_timeout);

  if (torrent::dht_controller()->is_active()) {
    LT_LOG_THIS("stopping server", 0);

    log_statistics(true);
    torrent::dht_controller()->stop();
  }
}

void
DhtManager::save_dht_cache() {
  if (!control->core()->download_store()->is_enabled() || !torrent::dht_controller()->is_valid())
    return;

  std::string filename = control->core()->download_store()->path() + "rtorrent.dht_cache";
  std::string filename_tmp = filename + ".new";
  std::fstream cache_file(filename_tmp.c_str(), std::ios::out | std::ios::trunc);

  if (!cache_file.is_open())
    return;

  torrent::Object cache = torrent::Object::create_map();
  cache_file << *torrent::dht_controller()->store_cache(&cache);

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
  if (!torrent::dht_controller()->is_active())
    throw torrent::internal_error("DhtManager::update called with DHT inactive.");

  if (m_start == dht_auto && !m_stop_timeout.is_scheduled()) {
    DownloadList::const_iterator itr, end;

    for (itr = control->core()->download_list()->begin(), end = control->core()->download_list()->end(); itr != end; ++itr)
      if ((*itr)->download()->info()->is_active() && !(*itr)->download()->info()->is_private())
        break;

    if (itr == end) {
      m_stop_timeout.slot() = [this] { stop_dht(); };
      torrent::this_thread::scheduler()->wait_for_ceil_seconds(&m_stop_timeout, 15min);
    }
  }

  // While bootstrapping (log_statistics returns true), check every minute if it completed, otherwise update every 15 minutes.
  if (log_statistics(false))
    torrent::this_thread::scheduler()->wait_for_ceil_seconds(&m_update_timeout, 1min);
  else
    torrent::this_thread::scheduler()->wait_for_ceil_seconds(&m_update_timeout, 15min);
}

bool
DhtManager::log_statistics(bool force) {
  auto stats = torrent::dht_controller()->get_statistics();

  // Check for firewall problems.

  if (stats.cycle > 2 && stats.queries_sent - m_dhtPrevQueriesSent > 100 && stats.queries_received == m_dhtPrevQueriesReceived) {
    // We should have had clients ping us at least but have received
    // nothing, that means the UDP port is probably unreachable.
    if (torrent::dht_controller()->is_receiving_requests())
      LT_LOG_THIS("listening port appears to be unreachable, no queries received", 0);

    torrent::dht_controller()->set_receive_requests(false);
  }

  if (stats.queries_sent - m_dhtPrevQueriesSent > stats.num_nodes * 2 + 20 && stats.replies_received == m_dhtPrevRepliesReceived) {
    // No replies to over 20 queries plus two per node we have. Probably firewalled.
    if (!m_warned)
      LT_LOG_THIS("listening port appears to be firewalled, no replies received", 0);

    m_warned = true;
    return false;
  }

  m_warned = false;

  if (stats.queries_received > m_dhtPrevQueriesReceived)
    torrent::dht_controller()->set_receive_requests(true);

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
  dhtStats.insert_key("active",           torrent::dht_controller()->is_active());
  dhtStats.insert_key("throttle",         m_throttleName);

  if (torrent::dht_controller()->is_active()) {
    auto stats = torrent::dht_controller()->get_statistics();

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
  if (torrent::dht_controller()->is_active())
    throw torrent::input_error("Cannot set DHT throttle while active.");

  m_throttleName = throttleName;
}

}
