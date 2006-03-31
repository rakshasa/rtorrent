// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
#include <sstream>
#include <iomanip>
#include <rak/socket_address.h>
#include <rak/timer.h>
#include <torrent/exceptions.h>
#include <torrent/connection_manager.h>
#include <torrent/rate.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>

#include "core/download.h"

#include "utils.h"

namespace display {

char*
print_string(char* first, char* last, char* str) {
  // We don't have any nice simple functions for copying strings that
  // return the end address.
  while (first != last && *str != '\0')
    *(first++) = *(str++);

  return first;
}

char*
print_hhmmss(char* first, char* last, time_t t) {
  std::tm *u = std::localtime(&t);
  
  if (u == NULL)
    //return "inv_time";
    throw torrent::internal_error("print_hhmmss(...) failed.");

  return print_buffer(first, last, "%2u:%02u:%02u", u->tm_hour, u->tm_min, u->tm_sec);
}

char*
print_ddhhmm(char* first, char* last, time_t t) {
  if (t / (24 * 3600) < 100)
    return print_buffer(first, last, "%2id %2i:%02i", (int)t / (24 * 3600), ((int)t / 3600) % 24, ((int)t / 60) % 60);
  else
    return print_buffer(first, last, "--:--:--");
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
  return print_buffer(first, last, " %s", d->download()->name().c_str());
}

char*
print_download_info(char* first, char* last, core::Download* d) {
  first = print_buffer(first, last, "Torrent: ");

  if (!d->download()->is_open())
    first = print_buffer(first, last, "closed            ");

  else if (d->is_done())
    first = print_buffer(first, last, "done %10.1f MB", (double)d->download()->bytes_total() / (double)(1 << 20));
  else
    first = print_buffer(first, last, "%6.1f / %6.1f MB",
		       (double)d->download()->bytes_done() / (double)(1 << 20),
		       (double)d->download()->bytes_total() / (double)(1 << 20));
  
  first = print_buffer(first, last, " Rate: %5.1f / %5.1f KB Uploaded: %7.1f MB",
		     (double)d->download()->up_rate()->rate() / (1 << 10),
		     (double)d->download()->down_rate()->rate() / (1 << 10),
		     (double)d->download()->up_rate()->total() / (1 << 20));

  if (d->download()->is_active() && !d->is_done()) {
    first = print_buffer(first, last, " ");
    first = print_download_percentage_done(first, last, d);

    first = print_buffer(first, last, " ");
    first = print_download_time_left(first, last, d);
  } else {
    first = print_buffer(first, last, "               ");
  }

  if (d->priority() != 2)
    first = print_buffer(first, last, " [%s]", core::Download::priority_to_string(d->priority()));

  if (first > last)
    throw torrent::internal_error("print_download_info(...) wrote past end of the buffer.");

  return first;
}

char*
print_download_status(char* first, char* last, core::Download* d) {
  if (!d->download()->is_active())
    first = print_buffer(first, last, "Inactive: ");

  if (d->download()->is_hash_checking()) {
    first = print_buffer(first, last, "Checking hash [%2i%%]",
		       (d->download()->chunks_hashed() * 100) / d->download()->chunks_total());

  } else if (d->tracker_list()->is_busy() && d->tracker_list()->focus() < d->tracker_list()->size()) {
    torrent::TrackerList* tl = d->tracker_list();

    first = print_buffer(first, last, "Tracker[%i:%i]: Connecting to %s",
			 tl->get(tl->focus()).group(), tl->focus(), tl->get(tl->focus()).url().c_str());

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
  uint32_t rate = d->download()->down_rate()->rate();

  if (rate < 512)
    return print_buffer(first, last, "--:--:--");
  
  time_t remaining = (d->download()->bytes_total() - d->download()->bytes_done()) / (rate & ~(uint32_t)(512 - 1));

  return print_ddhhmm(first, last, remaining);
}

char*
print_download_percentage_done(char* first, char* last, core::Download* d) {
  if (!d->is_open() || d->is_done())
    //return print_buffer(first, last, "[--%%]");
    return print_buffer(first, last, "     ");
  else
    return print_buffer(first, last, "[%2u%%]", (d->download()->chunks_done() * 100) / d->download()->chunks_total());
}

char*
print_status_info(char* first, char* last) {
  if (torrent::up_throttle() == 0)
    first = print_buffer(first, last, "[Throttle off");
  else
    first = print_buffer(first, last, "[Throttle %3i", torrent::up_throttle() / 1024);

  if (torrent::down_throttle() == 0)
    first = print_buffer(first, last, "/off KB]");
  else
    first = print_buffer(first, last, "/%3i KB]", torrent::down_throttle() / 1024);
  
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
print_status_extra(char* first, char* last, __UNUSED Control* c) {
  first = print_buffer(first, last, " [U %i/%i]",
		       torrent::currently_unchoked(),
		       torrent::max_unchoked());

  first = print_buffer(first, last, " [S %i/%i/%i]",
		       torrent::total_handshakes(),
		       torrent::open_sockets(),
		       torrent::max_open_sockets());
		       
  first = print_buffer(first, last, " [F %i/%i]",
		       torrent::open_files(),
		       torrent::max_open_files());

  return first;
}

}
