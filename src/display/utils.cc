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

#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>
#include <rak/socket_address.h>
#include <rak/timer.h>
#include <torrent/exceptions.h>
#include <torrent/connection_manager.h>
#include <torrent/rate.h>
#include <torrent/throttle.h>
#include <torrent/torrent.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>
#include <torrent/data/file_list.h>
#include <torrent/data/file_manager.h>
#include <torrent/download/resource_manager.h>
#include <torrent/peer/client_info.h>

#include "core/download.h"
#include "core/manager.h"
#include "rpc/parse_commands.h"

#include "control.h"
#include "globals.h"
#include "utils.h"

namespace display {

char*
print_string(char* first, char* last, char* str) {
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

char*
print_address(char* first, char* last, const rak::socket_address* sa) {
  if (!sa->address_c_str(first, last - first))
    return first;

  return std::find(first, last, '\0');
}

inline char*
print_address(char* first, char* last, const sockaddr* sa) {
  return print_address(first, last, rak::socket_address::cast_from(sa));
}

char*
print_download_title(char* first, char* last, core::Download* d) {
  return print_buffer(first, last, " %s", d->info()->name().c_str());
}

char*
print_download_info(char* first, char* last, core::Download* d) {
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
    throw torrent::internal_error("print_download_info(...) wrote past end of the buffer.");

  return first;
}

char*
print_download_info2(char* first, char* last, core::Download* d) {
  //Name
  std::string name = "Name";
  if(d) name = d->info()->name();
  name.resize(64,' ');
  first = print_buffer(first, last, " %s", name.c_str());

  //Status
  first = print_buffer(first, last, "|");
  if (!d)
    first = print_buffer(first, last, " Status ");
  else if (!d->download()->info()->is_open())
    first = print_buffer(first, last, " CLOSED ");
  else if (!d->download()->info()->is_active())
    first = print_buffer(first, last, " OPEN   ");
  else
    first = print_buffer(first, last, "        ");

  //Downloaded
  first = print_buffer(first, last, "|");
  if (!d)
    first = print_buffer(first, last, " Downloaded ");
  else
    first = print_buffer(first, last, " %7.1f MB ", (double)d->download()->bytes_done() / (double)(1 << 20));

  //Size
  first = print_buffer(first, last, "|");
  if (!d)
    first = print_buffer(first, last, " Size       ");
  else
    first = print_buffer(first, last, " %7.1f MB ", (double)d->download()->file_list()->size_bytes() / (double)(1 << 20));

  //Done
  first = print_buffer(first, last, "|");
  if (!d)
    first = print_buffer(first, last, " Done ");
  else if (d->is_done())
    first = print_buffer(first, last, " 100%% ");
  else if (d->is_open())
    first = print_buffer(first, last, "  %2u%% ",(d->download()->file_list()->completed_chunks() * 100) / d->download()->file_list()->size_chunks());
  else
    first = print_buffer(first, last, "      ");

  //Rate Up
  first = print_buffer(first, last, "|");
  if (!d)
    first = print_buffer(first, last, " Up Rate   ");
  else
    first = print_buffer(first, last, " %6.1f KB ", (double)d->info()->up_rate()->rate() / (1 << 10));

  //Rate Down
  first = print_buffer(first, last, "|");
  if (!d)
    first = print_buffer(first, last, " Down Rate ");
  else
    first = print_buffer(first, last, " %6.1f KB ", (double)d->info()->down_rate()->rate() / (1 << 10));

  //Uploaded
  first = print_buffer(first, last, "|");
  if (!d)
    first = print_buffer(first, last, " Uploaded   ");
  else
    first = print_buffer(first, last, " %7.1f MB ", (double)d->info()->up_rate()->total() / (1 << 20));

  //ETA
  first = print_buffer(first, last, "| ");
  if (!d)
    first = print_buffer(first, last, " ETA     ");
  else if (d->download()->info()->is_active() && !d->is_done())
    first = print_download_time_left(first, last, d);
  else
    first = print_buffer(first, last, "         ");

  //Ratio
  first = print_buffer(first, last, " |");
  if (!d)
    first = print_buffer(first, last, " Ratio");
  else
    first = print_buffer(first, last, " %4.2f ", (double)rpc::call_command_value("d.ratio", rpc::make_target(d)) / 1000.0);

  //Misc
  first = print_buffer(first, last, "|");
  if (!d) {
    first = print_buffer(first, last, " Misc ");
  } else {
    first = print_buffer(first, last, " %c%c",
                         rpc::call_command_string("d.tied_to_file", rpc::make_target(d)).empty() ? ' ' : 'T',
                         rpc::call_command_value("d.ignore_commands", rpc::make_target(d)) == 0 ? ' ' : 'I',
                         (double)rpc::call_command_value("d.ratio", rpc::make_target(d)) / 1000.0);

    if (d->priority() != 2)
      first = print_buffer(first, last, " %s", rpc::call_command_string("d.priority_str", rpc::make_target(d)).c_str());

    if (!d->bencode()->get_key("rtorrent").get_key_string("throttle_name").empty())
      first = print_buffer(first, last , " %s", rpc::call_command_string("d.throttle_name", rpc::make_target(d)).c_str());
  }

  if (first > last)
    throw torrent::internal_error("print_download_info(...) wrote past end of the buffer.");

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

  } else if (d->tracker_list()->has_active_not_scrape()) {
    torrent::TrackerList::iterator itr =
      std::find_if(d->tracker_list()->begin(), d->tracker_list()->end(),
                   std::mem_fun(&torrent::Tracker::is_busy_not_scrape));
    char status[128];

    (*itr)->get_status(status, sizeof(status));
    first = print_buffer(first, last, "Tracker[%i:%i]: Connecting to %s %s",
                         (*itr)->group(), std::distance(d->tracker_list()->begin(), itr), (*itr)->url().c_str(), status);

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
print_status_info(char* first, char* last) {
  if (!torrent::up_throttle_global()->is_throttled())
    first = print_buffer(first, last, "[Throttle off");
  else
    first = print_buffer(first, last, "[Throttle %3i", torrent::up_throttle_global()->max_rate() / 1024);

  if (!torrent::down_throttle_global()->is_throttled())
    first = print_buffer(first, last, "/off KB]");
  else
    first = print_buffer(first, last, "/%3i KB]", torrent::down_throttle_global()->max_rate() / 1024);
  
  first = print_buffer(first, last, " [Rate %5.1f/%5.1f KB]",
                       (double)torrent::up_rate()->rate() / 1024.0,
                       (double)torrent::down_rate()->rate() / 1024.0);

  first = print_buffer(first, last, " [Port: %i]", (unsigned int)torrent::connection_manager()->listen_port());

  if (!rak::socket_address::cast_from(torrent::connection_manager()->local_address())->is_address_any()) {
    first = print_buffer(first, last, " [Local ");
    first = print_address(first, last, torrent::connection_manager()->local_address());
    first = print_buffer(first, last, "]");
  }
  
  if (first > last)
    throw torrent::internal_error("print_status_info(...) wrote past end of the buffer.");

  if (!rak::socket_address::cast_from(torrent::connection_manager()->bind_address())->is_address_any()) {
    first = print_buffer(first, last, " [Bind ");
    first = print_address(first, last, torrent::connection_manager()->bind_address());
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
                       control->core()->http_stack()->active(),
                       control->core()->http_stack()->max_active());                       

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
