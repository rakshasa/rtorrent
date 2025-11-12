#include "config.h"

#include <cassert>
#include <cstdio>
#include <netdb.h>
#include <torrent/net/network_config.h>
#include <torrent/net/network_manager.h>
#include <torrent/net/resolver.h>
#include <torrent/tracker/dht_controller.h>
#include <torrent/tracker/tracker.h>
#include <torrent/utils/log.h>

#include "core/download.h"
#include "core/manager.h"

#include "globals.h"
#include "control.h"
#include "command_helpers.h"
#include "core/dht_manager.h"

void
tracker_set_enabled(torrent::tracker::Tracker* tracker, bool state) {
  if (state)
    tracker->enable();
  else
    tracker->disable();
}

torrent::Object
apply_dht_add_node(const std::string& arg) {
  if (!torrent::runtime::network_manager()->is_dht_valid())
    throw torrent::input_error("DHT not enabled.");

  int port;
  char dummy;
  char host[1024];

  int ret = std::sscanf(arg.c_str(), "%1023[^:]:%i%c", host, &port, &dummy);

  if (ret == 1)
    port = 6881;
  else if (ret != 2)
    throw torrent::input_error("Could not parse host.");

  if (port < 1 || port > 65535)
    throw torrent::input_error("Invalid port number.");

  assert(std::this_thread::get_id() == torrent::main_thread::thread()->thread_id());

  auto host_str = std::string(host);

  // TODO: Move this lookup to DhtController.

  // Currently discarding SOCK_STREAM.
  torrent::this_thread::resolver()->resolve_specific(nullptr, host_str, PF_INET, [host_str, port](torrent::c_sa_shared_ptr sa, int err) {
      if (sa == nullptr) {
        lt_log_print(torrent::LOG_DHT_ERROR, "dht.add_node : could not resolve host : %s (%s)", gai_strerror(err), host_str.c_str());
        return;
      }

      lt_log_print(torrent::LOG_DHT_CONTROLLER, "dht.add_node : %s", host_str.c_str());
      torrent::runtime::network_manager()->dht_add_peer_node(sa.get(), port);
  });

  return torrent::Object();
}

torrent::Object
apply_enable_trackers(int64_t arg) {
  if (arg == 0) {
    for (auto download : *control->core()->download_list())
      download->tracker_controller().for_each([](auto& tracker) { tracker.disable(); });

  } else if (rpc::call_command_value("trackers.use_udp") == 0) {
    for (auto download : *control->core()->download_list()) {
      download->tracker_controller().for_each([](auto& tracker) {
        if (tracker.type() == torrent::TRACKER_UDP)
          tracker.disable();
        else
          tracker.enable();
      });
    }

  } else {
    for (auto download : *control->core()->download_list())
      download->tracker_controller().for_each([](auto& tracker) { tracker.enable(); });
  }

  return torrent::Object();
}

void
initialize_command_tracker() {
  CMD2_TRACKER        ("t.is_busy",           std::bind(&torrent::tracker::Tracker::is_busy, std::placeholders::_1));
  CMD2_TRACKER        ("t.is_enabled",        std::bind(&torrent::tracker::Tracker::is_enabled, std::placeholders::_1));
  CMD2_TRACKER        ("t.is_extra_tracker",  std::bind(&torrent::tracker::Tracker::is_extra_tracker, std::placeholders::_1));
  CMD2_TRACKER        ("t.is_open",           std::bind(&torrent::tracker::Tracker::is_busy, std::placeholders::_1));
  CMD2_TRACKER        ("t.is_scrapable",      std::bind(&torrent::tracker::Tracker::is_scrapable, std::placeholders::_1));
  CMD2_TRACKER        ("t.is_usable",         std::bind(&torrent::tracker::Tracker::is_usable, std::placeholders::_1));

  // TODO: Deprecate.
  CMD2_TRACKER        ("t.can_scrape",        std::bind(&torrent::tracker::Tracker::is_scrapable, std::placeholders::_1));

  CMD2_TRACKER_V      ("t.enable",            std::bind(&torrent::tracker::Tracker::enable, std::placeholders::_1));
  CMD2_TRACKER_V      ("t.disable",           std::bind(&torrent::tracker::Tracker::disable, std::placeholders::_1));

  CMD2_TRACKER_VALUE_V("t.is_enabled.set",    std::bind(&tracker_set_enabled, std::placeholders::_1, std::placeholders::_2));

  CMD2_TRACKER        ("t.url",               std::bind(&torrent::tracker::Tracker::url, std::placeholders::_1));
  CMD2_TRACKER        ("t.group",             std::bind(&torrent::tracker::Tracker::group, std::placeholders::_1));
  CMD2_TRACKER        ("t.type",              std::bind(&torrent::tracker::Tracker::type, std::placeholders::_1));
  CMD2_TRACKER        ("t.id",                std::bind(&torrent::tracker::Tracker::tracker_id, std::placeholders::_1));

  CMD2_TRACKER        ("t.latest_event",      [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().latest_event(); });
  CMD2_TRACKER        ("t.latest_new_peers",  [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().latest_new_peers(); });
  CMD2_TRACKER        ("t.latest_sum_peers",  [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().latest_sum_peers(); });

  CMD2_TRACKER        ("t.normal_interval",   [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().normal_interval(); });
  CMD2_TRACKER        ("t.min_interval",      [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().min_interval(); });

  CMD2_TRACKER        ("t.activity_time_next", [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().activity_time_next(); });
  CMD2_TRACKER        ("t.activity_time_last", [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().activity_time_last(); });

  CMD2_TRACKER        ("t.success_time_next", [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().success_time_next(); });
  CMD2_TRACKER        ("t.success_time_last", [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().success_time_last(); });
  CMD2_TRACKER        ("t.success_counter",   [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().success_counter(); });

  CMD2_TRACKER        ("t.failed_time_next",  [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().failed_time_next(); });
  CMD2_TRACKER        ("t.failed_time_last",  [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().failed_time_last(); });
  CMD2_TRACKER        ("t.failed_counter",    [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().failed_counter(); });

  CMD2_TRACKER        ("t.scrape_time_last",  [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().scrape_time_last(); });
  CMD2_TRACKER        ("t.scrape_counter",    [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().scrape_counter(); });

  CMD2_TRACKER        ("t.scrape_complete",   [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().scrape_complete(); });
  CMD2_TRACKER        ("t.scrape_incomplete", [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().scrape_incomplete(); });
  CMD2_TRACKER        ("t.scrape_downloaded", [](torrent::tracker::Tracker* tracker, [[maybe_unused]] auto o) -> auto { return tracker->state().scrape_downloaded(); });

  CMD2_ANY_VALUE      ("trackers.enable",     std::bind(&apply_enable_trackers, int64_t(1)));
  CMD2_ANY_VALUE      ("trackers.disable",    std::bind(&apply_enable_trackers, int64_t(0)));
  CMD2_VAR_BOOL       ("trackers.delay_scrape", false);
  CMD2_VAR_VALUE      ("trackers.numwant",    -1);
  CMD2_VAR_BOOL       ("trackers.use_udp",    true);

  auto dht_manager = control->dht_manager();

  CMD2_ANY_STRING_V   ("dht.mode.set",          std::bind(&core::DhtManager::set_mode, control->dht_manager(), std::placeholders::_2));
  CMD2_ANY            ("dht.port",              [](auto, auto) { return torrent::runtime::network_manager()->dht_controller()->port(); });
  CMD2_ANY_VALUE_V    ("dht.port.set",          [](auto, auto) {
      lt_log_print(torrent::LOG_DHT_ERROR, "dht.port.set is no longer supported, use dht.override_port.set", 0);
    });
  CMD2_ANY            ("dht.override_port",     std::bind(&torrent::net::NetworkConfig::override_dht_port, torrent::config::network_config()));
  CMD2_ANY_VALUE_V    ("dht.override_port.set", std::bind(&torrent::net::NetworkConfig::set_override_dht_port, torrent::config::network_config(), std::placeholders::_2));

  CMD2_ANY_STRING     ("dht.add_node",          std::bind(&apply_dht_add_node, std::placeholders::_2));
  CMD2_ANY            ("dht.statistics",        std::bind(&core::DhtManager::dht_statistics, dht_manager));
}
