#include "config.h"

#include <cassert>
#include <cstdio>
#include <netdb.h>
#include <torrent/net/resolver.h>
#include <torrent/runtime/network_config.h>
#include <torrent/runtime/network_manager.h>
#include <torrent/system/callbacks.h>
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

  assert(std::this_thread::get_id() == torrent::main_thread::thread_id());

  auto host_str = std::string(host);

  // TODO: Move this lookup to DhtController.

  auto callback_id = torrent::system::make_callback_id();

  // Currently discarding SOCK_STREAM.
  torrent::this_thread::resolver()->resolve_specific(callback_id, host_str, PF_INET, [host_str, port](torrent::c_sa_shared_ptr sa, int err) {
      if (sa == nullptr) {
        lt_log_print(torrent::LOG_DHT_ERROR, "dht.add_node : could not resolve host : %s (%s)", gai_strerror(err), host_str.c_str());
        return;
      }

      lt_log_print(torrent::LOG_DHT_CONTROLLER, "dht.add_node : %s", host_str.c_str());
      torrent::runtime::network_manager()->dht_add_bootstrap_node(host_str.c_str(), port);
  });

  return torrent::Object();
}

torrent::Object
apply_enable_trackers(int64_t arg) {
  if (arg == 0) {
    for (auto download : *control->core()->download_list())
      download->tracker_controller().for_each([](auto& tracker) { tracker.disable(); });

  } else {
    for (auto download : *control->core()->download_list())
      download->tracker_controller().for_each([](auto& tracker) { tracker.enable(); });
  }

  return torrent::Object();
}

void
initialize_command_tracker() {
  CMD2_TRACKER        ("t.is_busy",            [](auto* tracker, auto) { return tracker->is_requesting(); });
  CMD2_TRACKER        ("t.is_enabled",         [](auto* tracker, auto) { return tracker->is_enabled(); });
  CMD2_TRACKER        ("t.is_extra_tracker",   [](auto* tracker, auto) { return tracker->is_extra_tracker(); });
  CMD2_TRACKER        ("t.is_open",            [](auto* tracker, auto) { return tracker->is_requesting(); });
  CMD2_TRACKER        ("t.is_scrapable",       [](auto* tracker, auto) { return tracker->is_scrapable(); });
  CMD2_TRACKER        ("t.is_usable",          [](auto* tracker, auto) { return tracker->is_usable(); });

  // TODO: Deprecate.
  CMD2_TRACKER        ("t.can_scrape",         [](auto* tracker, auto) { return tracker->is_scrapable(); });

  CMD2_TRACKER_V      ("t.enable",             [](auto* tracker, auto) { tracker->enable(); });
  CMD2_TRACKER_V      ("t.disable",            [](auto* tracker, auto) { tracker->disable(); });

  CMD2_TRACKER_VALUE_V("t.is_enabled.set",     [](auto* tracker, auto& value) { return tracker_set_enabled(tracker, value); });

  CMD2_TRACKER        ("t.url",                [](auto* tracker, auto) { return tracker->url(); });
  CMD2_TRACKER        ("t.group",              [](auto* tracker, auto) { return tracker->group(); });
  CMD2_TRACKER        ("t.type",               [](auto* tracker, auto) { return tracker->type(); });
  CMD2_TRACKER        ("t.id",                 [](auto* tracker, auto) { return tracker->tracker_id(); });

  CMD2_TRACKER        ("t.latest_event",       [](auto* tracker, auto) { return tracker->state().latest_event(); });
  CMD2_TRACKER        ("t.latest_new_peers",   [](auto* tracker, auto) { return tracker->state().latest_new_peers(); });
  CMD2_TRACKER        ("t.latest_sum_peers",   [](auto* tracker, auto) { return tracker->state().latest_sum_peers(); });

  CMD2_TRACKER        ("t.normal_interval",    [](auto* tracker, auto) { return tracker->state().normal_interval().count(); });
  CMD2_TRACKER        ("t.min_interval",       [](auto* tracker, auto) { return tracker->state().min_interval().count(); });

  CMD2_TRACKER        ("t.activity_time_next", [](auto* tracker, auto) { return tracker->state().activity_time_next().count(); });
  CMD2_TRACKER        ("t.activity_time_last", [](auto* tracker, auto) { return tracker->state().activity_time_last().count(); });

  CMD2_TRACKER        ("t.success_time_next",  [](auto* tracker, auto) { return tracker->state().success_time_next().count(); });
  CMD2_TRACKER        ("t.success_time_last",  [](auto* tracker, auto) { return tracker->state().success_time_last().count(); });
  CMD2_TRACKER        ("t.success_counter",    [](auto* tracker, auto) { return tracker->state().success_counter(); });

  CMD2_TRACKER        ("t.failed_time_next",   [](auto* tracker, auto) { return tracker->state().failed_time_next().count(); });
  CMD2_TRACKER        ("t.failed_time_last",   [](auto* tracker, auto) { return tracker->state().failed_time_last().count(); });
  CMD2_TRACKER        ("t.failed_counter",     [](auto* tracker, auto) { return tracker->state().failed_counter(); });

  CMD2_TRACKER        ("t.scrape_time_last",   [](auto* tracker, auto) { return tracker->state().scrape_time_last().count(); });
  CMD2_TRACKER        ("t.scrape_counter",     [](auto* tracker, auto) { return tracker->state().scrape_counter(); });

  CMD2_TRACKER        ("t.scrape_complete",    [](auto* tracker, auto) { return tracker->state().scrape_complete(); });
  CMD2_TRACKER        ("t.scrape_incomplete",  [](auto* tracker, auto) { return tracker->state().scrape_incomplete(); });
  CMD2_TRACKER        ("t.scrape_downloaded",  [](auto* tracker, auto) { return tracker->state().scrape_downloaded(); });

  CMD2_ANY_VALUE      ("trackers.enable",      [](auto, auto)          { return apply_enable_trackers(1); });
  CMD2_ANY_VALUE      ("trackers.disable",     [](auto, auto)          { return apply_enable_trackers(0); });
  CMD2_VAR_BOOL       ("trackers.delay_scrape", false);
  CMD2_VAR_VALUE      ("trackers.numwant",      -1);

  CMD2_ANY            ("trackers.use_udp",     [](auto, auto) { return true; });
  CMD2_ANY_VALUE_V    ("trackers.use_udp.set", [](auto, auto) {
      lt_log_print(torrent::LOG_ERROR, "trackers.use_udp.set is no longer supported", 0);
    })

  CMD2_ANY_STRING_V   ("dht.mode.set",          [](auto, auto& str) { return control->dht_manager()->set_mode_by_user(str); });
  CMD2_ANY            ("dht.port",              [](auto, auto)      { return torrent::runtime::network_manager()->dht_controller()->port(); });
  CMD2_ANY_VALUE_V    ("dht.port.set",          [](auto, auto) {
      lt_log_print(torrent::LOG_DHT_ERROR, "dht.port.set is no longer supported, use dht.override_port.set", 0);
    });
  CMD2_ANY            ("dht.override_port",     [](auto, auto)        { return torrent::runtime::network_config()->override_dht_port(); });
  CMD2_ANY_VALUE_V    ("dht.override_port.set", [](auto, auto& value) { return torrent::runtime::network_manager()->set_dht_port(value); });

  CMD2_ANY_STRING     ("dht.add_node",          [](auto, auto& str)   { return apply_dht_add_node(str); });
  CMD2_ANY            ("dht.statistics",        [](auto, auto)        { return control->dht_manager()->dht_statistics(); });

  rpc::rpc.mark_safe("t.url");
  rpc::rpc.mark_safe("t.group");
  rpc::rpc.mark_safe("t.id");
  rpc::rpc.mark_safe("t.type");
  rpc::rpc.mark_safe("t.is_usable");
  rpc::rpc.mark_safe("t.is_busy");
  rpc::rpc.mark_safe("t.is_enabled");
  rpc::rpc.mark_safe("t.is_enabled.set");
  rpc::rpc.mark_safe("t.is_extra_tracker");
  rpc::rpc.mark_safe("t.is_open");
  rpc::rpc.mark_safe("t.normal_interval");
  rpc::rpc.mark_safe("t.scrape_time_last");
  rpc::rpc.mark_safe("t.scrape_counter");
  rpc::rpc.mark_safe("t.success_time_last");
  rpc::rpc.mark_safe("t.success_counter");
  rpc::rpc.mark_safe("t.failed_time_last");
  rpc::rpc.mark_safe("t.failed_counter");
  rpc::rpc.mark_safe("t.activity_time_last");
  rpc::rpc.mark_safe("t.activity_time_next");
  rpc::rpc.mark_safe("t.scrape_complete");
  rpc::rpc.mark_safe("t.scrape_incomplete");
  rpc::rpc.mark_safe("t.scrape_downloaded");

  rpc::rpc.mark_safe("dht.mode.set");
  rpc::rpc.mark_safe("dht.port");
  rpc::rpc.mark_safe("dht.override_port");
  rpc::rpc.mark_safe("dht.add_node");
  rpc::rpc.mark_safe("dht.statistics");
  rpc::rpc.mark_safe("trackers.numwant");
  rpc::rpc.mark_safe("trackers.use_udp");
}
