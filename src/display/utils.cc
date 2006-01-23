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
#include <torrent/rate.h>
#include <torrent/tracker.h>

#include "core/download.h"
#include <rak/timer.h>

#include "utils.h"

namespace display {

char*
print_string(char* buf, unsigned int length, char* str) {
  // We don't have any nice simple functions for copying strings that
  // return the end address.
  while (length-- != 0 && *str != '\0')
    *(buf++) = *(str++);

  return buf;
}

char*
print_hhmmss(char* buf, unsigned int length, time_t t) {
  std::tm *u = std::localtime(&t);
  
  if (u == NULL)
    return "inv_time";

  int s = snprintf(buf, length, "%2u:%02u:%02u", u->tm_hour, u->tm_min, u->tm_sec);

  return buf + std::max(s, 0);
}

char*
print_ddhhmm(char* buf, unsigned int length, time_t t) {
  int s;

  if (t / (24 * 3600) < 100)
    s = snprintf(buf, length, "%2i:%02i:%02i", (int)t / (24 * 3600), ((int)t / 3600) % 24, ((int)t / 60) % 60);
  else
    s = snprintf(buf, length, "--:--:--");
  
  return buf + std::max(s, 0);
}

char*
print_ddmmyyyy(char* buf, unsigned int length, time_t t) {
  std::tm *u = std::gmtime(&t);
  
  if (u == NULL)
    return "inv_time";

  int s = snprintf(buf, length, "%02u/%02u/%04u", u->tm_mday, (u->tm_mon + 1), (1900 + u->tm_year));

  return buf + std::max(s, 0);
}

char*
print_download_title(char* buf, unsigned int length, core::Download* d) {
  return buf + std::max(snprintf(buf, length, "%s", d->get_download().name().c_str()), 0);
}

char*
print_download_info(char* buf, unsigned int length, core::Download* d) {
  char* last = buf + length;

  buf += std::max(snprintf(buf, last - buf, "Torrent: "), 0);

  if (!d->get_download().is_open())
    buf += std::max(snprintf(buf, last - buf, "closed            "), 0);
  else if (d->is_done())
    buf += std::max(snprintf(buf, last - buf, "done %10.1f MB",
			     (double)d->get_download().bytes_total() / (double)(1 << 20)), 0);
  else
    buf += std::max(snprintf(buf, last - buf, "%6.1f / %6.1f MB",
			     (double)d->get_download().bytes_done() / (double)(1 << 20),
			     (double)d->get_download().bytes_total() / (double)(1 << 20)), 0);
  
  buf += std::max(snprintf(buf, last - buf, " Rate: %5.1f / %5.1f KB Uploaded: %7.1f MB ",
			   (double)d->get_download().up_rate()->rate() / (1 << 10),
			   (double)d->get_download().down_rate()->rate() / (1 << 10),
			   (double)d->get_download().up_rate()->total() / (1 << 20)), 0);

  buf = print_download_time_left(buf, last - buf, d);

  *buf = '\0';

  return buf;
}

char*
print_download_status(char* buf, unsigned int length, core::Download* d) {
  char* last = buf + length;

  if (!d->get_download().is_active())
    buf += std::max(snprintf(buf, last - buf, "Inactive: "), 0);

  if (d->get_download().is_hash_checking())
    buf += std::max(snprintf(buf, last - buf, "Checking hash [%2i%%]",
			     (d->get_download().chunks_hashed() * 100) / d->get_download().chunks_total()), 0);

  else if (d->get_download().is_tracker_busy() &&
	   d->get_download().tracker_focus() < d->get_download().size_trackers())
    buf += std::max(snprintf(buf, last - buf, "Tracker[%i:%i]: Connecting to %s",
			     d->get_download().tracker(d->get_download().tracker_focus()).group(),
			     d->get_download().tracker_focus(),
			     d->get_download().tracker(d->get_download().tracker_focus()).url().c_str()), 0);

  else if (!d->get_message().empty())
    buf += std::max(snprintf(buf, last - buf, "%s", d->get_message().c_str()), 0);

  else
    buf[0] = '\0';

  return buf;
}

char*
print_download_time_left(char* buf, unsigned int length, core::Download* d) {
  uint32_t rate;

  if (!d->get_download().is_active() ||
      (rate = d->get_download().down_rate()->rate()) < 512) {
    buf += std::max(snprintf(buf, length, "--:--:--"), 0);
    return buf;
  }
  
  time_t remaining = (d->get_download().bytes_total() - d->get_download().bytes_done()) / (rate & ~(uint32_t)(512 - 1));

  return print_ddhhmm(buf, length, remaining);
}

// char*
// print_entry_tags(char* buf, unsigned int length) {
  
// }

// char*
// print_entry_file(char* buf, unsigned int length, const torrent::Entry& entry);

}
