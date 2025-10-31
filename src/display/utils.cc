#include "config.h"

#include "display/utils.h"

#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <torrent/exceptions.h>
#include <torrent/connection_manager.h>
#include <torrent/rate.h>
#include <torrent/throttle.h>
#include <torrent/torrent.h>
#include <torrent/tracker/tracker.h>
#include <torrent/data/file_list.h>
#include <torrent/data/file_manager.h>
#include <torrent/download/resource_manager.h>
#include <torrent/net/http_stack.h>
#include <torrent/net/network_config.h>
#include <torrent/net/network_manager.h>
#include <torrent/net/socket_address.h>
#include <torrent/peer/client_info.h>

#include "control.h"
#include "globals.h"
#include "core/download.h"
#include "core/manager.h"
#include "rpc/parse_commands.h"
#include "ui/root.h"

namespace display {

char*
print_string(char* first, char* last, const char* str) {
  if (first == last)
    return first;

  // We don't have any nice simple functions for copying strings that
  // return the end address.
  while (first + 1 != last && *str != '\0')
    *(first++) = *(str++);

  *first = '\0';

  return first;
}

char*
print_hhmmss(char* first, char* last, time_t t) {
  return print_buffer(first, last, "%2d:%02d:%02d", (int)t / 3600, ((int)t / 60) % 60, (int)t % 60);
}

char*
print_hhmmss_local(char* first, char* last, time_t t) {
  std::tm *u = std::localtime(&t);

  if (u == NULL)
    //return "inv_time";
    throw torrent::internal_error("print_hhmmss_local(...) failed.");

  return print_buffer(first, last, "%2u:%02u:%02u", u->tm_hour, u->tm_min, u->tm_sec);
}

char*
print_ddhhmm(char* first, char* last, time_t t) {
  if (t / (24 * 3600) < 100)
    return print_buffer(first, last, "%2id %2i:%02i", (int)t / (24 * 3600), ((int)t / 3600) % 24, ((int)t / 60) % 60);
  else
    return print_buffer(first, last, "--d --:--");
}

char*
print_ddmmyyyy(char* first, char* last, time_t t) {
  std::tm *u = std::gmtime(&t);

  if (u == NULL)
    //return "inv_time";
    throw torrent::internal_error("print_ddmmyyyy(...) failed.");

  return print_buffer(first, last, "%02u/%02u/%04u", u->tm_mday, (u->tm_mon + 1), (1900 + u->tm_year));
}

inline char*
print_address(char* first, char* last, const sockaddr* sa) {
  return print_string(first, last, torrent::sa_addr_str(sa).c_str());
}

char*
print_download_title(char* first, char* last, core::Download* d) {
  return print_buffer(first, last, " %s", d->info()->name().c_str());
}

char*
print_download_info_full(char* first, char* last, core::Download* d) {
  if (!d->download()->info()->is_open())
    first = print_buffer(first, last, "[CLOSED]  ");
  else if (!d->download()->info()->is_active())
    first = print_buffer(first, last, "[OPEN]    ");
  else
    first = print_buffer(first, last, "          ");

  if (d->is_done())
    first = print_buffer(first, last, "done %10.1f MB", (double)d->download()->file_list()->size_bytes() / (double)(1 << 20));
  else
    first = print_buffer(first, last, "%6.1f / %6.1f MB",
                         (double)d->download()->bytes_done() / (double)(1 << 20),
                         (double)d->download()->file_list()->size_bytes() / (double)(1 << 20));

  first = print_buffer(first, last, " Rate: %5.1f / %5.1f KB Uploaded: %7.1f MB",
                       (double)d->info()->up_rate()->rate() / (1 << 10),
                       (double)d->info()->down_rate()->rate() / (1 << 10),
                       (double)d->info()->up_rate()->total() / (1 << 20));

  if (d->download()->info()->is_active() && !d->is_done()) {
    first = print_buffer(first, last, " ");
    first = print_download_percentage_done(first, last, d);

    first = print_buffer(first, last, " ");
    first = print_download_time_left(first, last, d);
  } else {
    first = print_buffer(first, last, "                ");
  }

  first = print_buffer(first, last, " [%c%c R: %4.2f",
                       rpc::call_command_string("d.tied_to_file", rpc::make_target(d)).empty() ? ' ' : 'T',
                       rpc::call_command_value("d.ignore_commands", rpc::make_target(d)) == 0 ? ' ' : 'I',
                       (double)rpc::call_command_value("d.ratio", rpc::make_target(d)) / 1000.0);

  if (d->priority() != 2)
    first = print_buffer(first, last, " %s", rpc::call_command_string("d.priority_str", rpc::make_target(d)).c_str());

  if (!d->bencode()->get_key("rtorrent").get_key_string("throttle_name").empty())
    first = print_buffer(first, last , " %s", rpc::call_command_string("d.throttle_name", rpc::make_target(d)).c_str());

  first = print_buffer(first, last , "]");

  if (first > last)
    throw torrent::internal_error("print_download_info_full(...) wrote past end of the buffer.");

  return first;
}

char*
print_download_status(char* first, char* last, core::Download* d) {
  if (d->is_active())
    ;
  else if (rpc::call_command_value("d.hashing", rpc::make_target(d)) != 0)
    first = print_buffer(first, last, "Hashing: ");
  else if (!d->is_active())
    first = print_buffer(first, last, "Inactive: ");

  if (d->is_hash_checking()) {
    first = print_buffer(first, last, "Checking hash [%2i%%]",
                         (d->download()->chunks_hashed() * 100) / d->download()->file_list()->size_chunks());

  } else if (d->tracker_controller().has_active_trackers_not_scrape()) {
    auto tracker = d->tracker_controller().find_if([](const auto& t) {
      return t.is_busy_not_scrape();
    });

    if (tracker.is_valid()) {
      auto status = tracker.status();

      first = print_buffer(first, last, "Tracker[%i]: Connecting to %s %s", tracker.group(), tracker.url().c_str(), status.c_str());
    } else {
      first = print_buffer(first, last, "Tracker: Connecting ...");
    }

  } else if (!d->message().empty()) {
    first = print_buffer(first, last, "%s", d->message().c_str());

  } else {
    *first = '\0';
  }

  if (first > last)
    throw torrent::internal_error("print_download_status(...) wrote past end of the buffer.");

  return first;
}

char*
print_download_column_compact(char* first, char* last) {
  first = print_buffer(first, last, " %-64.64s", "Name");
  first = print_buffer(first, last, "| Status | Downloaded |    Size    | Done |  Up Rate  | Down Rate |  Uploaded  |    ETA    | Ratio | Misc ");

  if (first > last)
    throw torrent::internal_error("print_download_column_compact(...) wrote past end of the buffer.");

  return first;
}

char*
print_download_info_compact(char* first, char* last, core::Download* d) {
  first = print_buffer(first, last, " %-64.64s", d->info()->name().c_str());
  first = print_buffer(first, last, "|");

  if (!d->download()->info()->is_open())
    first = print_buffer(first, last, " CLOSED ");
  else if (!d->download()->info()->is_active())
    first = print_buffer(first, last, "  OPEN  ");
  else
    first = print_buffer(first, last, "        ");

  first = print_buffer(first, last, "| %7.1f MB ", (double)d->download()->bytes_done() / (double)(1 << 20));
  first = print_buffer(first, last, "| %7.1f MB ", (double)d->download()->file_list()->size_bytes() / (double)(1 << 20));
  first = print_buffer(first, last, "|");

  if (d->is_done())
    first = print_buffer(first, last, " 100%% ");
  else if (d->is_open())
    first = print_buffer(first, last, "  %2u%% ",(d->download()->file_list()->completed_chunks() * 100) / d->download()->file_list()->size_chunks());
  else
    first = print_buffer(first, last, "      ");

  first = print_buffer(first, last, "| %6.1f KB ", (double)d->info()->up_rate()->rate() / (1 << 10));
  first = print_buffer(first, last, "| %6.1f KB ", (double)d->info()->down_rate()->rate() / (1 << 10));
  first = print_buffer(first, last, "| %7.1f MB ", (double)d->info()->up_rate()->total() / (1 << 20));
  first = print_buffer(first, last, "| ");

  if (d->download()->info()->is_active() && !d->is_done()) {
    first = print_download_time_left(first, last, d);
    first = print_buffer(first, last, " ");
  } else {
    first = print_buffer(first, last, "          ");
  }

  first = print_buffer(first, last, "| %5.2f ", (double)rpc::call_command_value("d.ratio", rpc::make_target(d)) / 1000.0);
  first = print_buffer(first, last, "| %c%c",
                       rpc::call_command_string("d.tied_to_file", rpc::make_target(d)).empty() ? ' ' : 'T',
                       rpc::call_command_value("d.ignore_commands", rpc::make_target(d)) == 0 ? ' ' : 'I',
                       (double)rpc::call_command_value("d.ratio", rpc::make_target(d)) / 1000.0);

  if (d->priority() != 2)
    first = print_buffer(first, last, " %s", rpc::call_command_string("d.priority_str", rpc::make_target(d)).c_str());

  if (!d->bencode()->get_key("rtorrent").get_key_string("throttle_name").empty())
    first = print_buffer(first, last , " %s", rpc::call_command_string("d.throttle_name", rpc::make_target(d)).c_str());

  if (first > last)
    throw torrent::internal_error("print_download_info_compact(...) wrote past end of the buffer.");

  return first;
}

char*
print_download_time_left(char* first, char* last, core::Download* d) {
  uint32_t rate = d->info()->down_rate()->rate();

  if (rate < 512)
    return print_buffer(first, last, "--d --:--");
  
  time_t remaining = (d->download()->file_list()->size_bytes() - d->download()->bytes_done()) / (rate & ~(uint32_t)(512 - 1));

  return print_ddhhmm(first, last, remaining);
}

char*
print_download_percentage_done(char* first, char* last, core::Download* d) {
  if (!d->is_open() || d->is_done())
    //return print_buffer(first, last, "[--%%]");
    return print_buffer(first, last, "     ");
  else
    return print_buffer(first, last, "[%2u%%]", (d->download()->file_list()->completed_chunks() * 100) / d->download()->file_list()->size_chunks());
}

char*
print_client_version(char* first, char* last, const torrent::ClientInfo& clientInfo) {
  switch (torrent::ClientInfo::version_size(clientInfo.type())) {
  case 4:
    return print_buffer(first, last, "%s %hhu.%hhu.%hhu.%hhu",
                        clientInfo.short_description(),
                        clientInfo.version()[0], clientInfo.version()[1],
                        clientInfo.version()[2], clientInfo.version()[3]);
  case 3:
    return print_buffer(first, last, "%s %hhu.%hhu.%hhu",
                        clientInfo.short_description(),
                        clientInfo.version()[0], clientInfo.version()[1],
                        clientInfo.version()[2]);
  default:
    return print_buffer(first, last, "%s", clientInfo.short_description());
  }
}

char*
print_status_throttle_limit(char* first, char* last, bool up, const ui::ThrottleNameList& throttle_names) {
  char throttle_str[40];
  throttle_str[0] = 0;
  char* firstc = throttle_str;
  char* lastc = throttle_str + 40 - 1;

  for (const auto& throttle_name : throttle_names) {

    if (!throttle_name.empty()) {
      int64_t throttle_max = control->core()->retrieve_throttle_value(throttle_name, false, up);

      if (throttle_max > 0)
        firstc = print_buffer(firstc, lastc, "|%1.0f", (double)throttle_max / 1024.0);
    }
  }

  // Add temp buffer (chop first char first) into main buffer if temp buffer isn't empty
  if (throttle_str[0] != 0)
    first = print_buffer(first, last, "(%s)", &throttle_str[1]);

  return first;
}

char*
print_status_throttle_rate(char* first, char* last, bool up, const ui::ThrottleNameList& throttle_names, const double& global_rate) {
  double main_rate = global_rate;
  char throttle_str[50];
  throttle_str[0] = 0;
  char* firstc = throttle_str;
  char* lastc = throttle_str + 50 - 1;

  for (const auto& throttle_name : throttle_names) {

    if (!throttle_name.empty() && (up ? torrent::up_throttle_global()->is_throttled() : torrent::down_throttle_global()->is_throttled())) {
      int64_t throttle_rate_value = control->core()->retrieve_throttle_value(throttle_name, true, up);

      if (throttle_rate_value > -1) {
        double throttle_rate = (double)throttle_rate_value / 1024.0;
        main_rate = main_rate - throttle_rate;

        firstc = print_buffer(firstc, lastc, "|%3.1f", throttle_rate);
      }
    }
  }

  // Add temp buffer into main buffer if temp buffer isn't empty
  if (throttle_str[0] != 0)
    first = print_buffer(first, last, "(%3.1f%s)",
                    main_rate < 0.0 ? 0.0 : main_rate,
                    throttle_str);

  return first;
}

char*
print_status_info(char* first, char* last) {
  ui::ThrottleNameList& throttle_up_names = control->ui()->get_status_throttle_up_names();
  ui::ThrottleNameList& throttle_down_names = control->ui()->get_status_throttle_down_names();

  if (!torrent::up_throttle_global()->is_throttled()) {
    first = print_buffer(first, last, "[Throttle off");
  } else {
    first = print_buffer(first, last, "[Throttle %3i", torrent::up_throttle_global()->max_rate() / 1024);

    if (!throttle_up_names.empty())
      first = print_status_throttle_limit(first, last, true, throttle_up_names);
  }

  if (!torrent::down_throttle_global()->is_throttled()) {
    first = print_buffer(first, last, " / off KB]");
  } else {
    first = print_buffer(first, last, " / %3i", torrent::down_throttle_global()->max_rate() / 1024);

    if (!throttle_down_names.empty())
      first = print_status_throttle_limit(first, last, false, throttle_down_names);

    first = print_buffer(first, last, " KB]");
  }

  double global_uprate = (double)torrent::up_rate()->rate() / 1024.0;
  first = print_buffer(first, last, " [Rate %5.1f", global_uprate);

  if (!throttle_up_names.empty())
    first = print_status_throttle_rate(first, last, true, throttle_up_names, global_uprate);

  double global_downrate = (double)torrent::down_rate()->rate() / 1024.0;
  first = print_buffer(first, last, " / %5.1f", global_downrate);

  if (!throttle_down_names.empty())
    first = print_status_throttle_rate(first, last, false, throttle_down_names, global_downrate);

  first = print_buffer(first, last, " KB]");

  first = print_buffer(first, last, " [Port: %i]", (unsigned int)torrent::runtime::network_manager()->listen_port());

  auto local_address = torrent::config::network_config()->local_address_best_match();

  if (!torrent::sa_is_any(local_address.get())) {
    first = print_buffer(first, last, " [Local ");
    first = print_address(first, last, local_address.get());
    first = print_buffer(first, last, "]");
  }

  if (first > last)
    throw torrent::internal_error("print_status_info(...) wrote past end of the buffer.");

  auto bind_inet_address  = torrent::config::network_config()->bind_inet_address();
  auto bind_inet6_address = torrent::config::network_config()->bind_inet6_address();
  bool show_bind_inet     = !torrent::sa_is_any(bind_inet_address.get());
  bool show_bind_inet6    = !torrent::sa_is_any(bind_inet6_address.get());

  if (show_bind_inet || show_bind_inet6) {
    first = print_buffer(first, last, " [Bind ");

    if (show_bind_inet)
      first = print_address(first, last, bind_inet_address.get());

    if (show_bind_inet6) {
      if (show_bind_inet)
        first = print_buffer(first, last, " / ");

      first = print_address(first, last, bind_inet6_address.get());
    }

    first = print_buffer(first, last, "]");
  }

  return first;
}

char*
print_status_extra(char* first, char* last) {
  first = print_buffer(first, last, " [U %i/%i]",
                       torrent::resource_manager()->currently_upload_unchoked(),
                       torrent::resource_manager()->max_upload_unchoked());

  first = print_buffer(first, last, " [D %i/%i]",
                       torrent::resource_manager()->currently_download_unchoked(),
                       torrent::resource_manager()->max_download_unchoked());

  first = print_buffer(first, last, " [H %u/%u]",
                       torrent::net_thread::http_stack()->size(),
                       torrent::net_thread::http_stack()->max_total_connections());

  first = print_buffer(first, last, " [S %i/%i/%i]",
                       torrent::total_handshakes(),
                       torrent::connection_manager()->size(),
                       torrent::connection_manager()->max_size());

  first = print_buffer(first, last, " [F %i/%i]",
                       torrent::file_manager()->open_files(),
                       torrent::file_manager()->max_open_files());

  return first;
}

}
