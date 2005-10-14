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

#include <sstream>
#include <iomanip>
#include <torrent/rate.h>
#include <torrent/tracker.h>

#include "core/download.h"
#include "utils/timer.h"

#include "utils.h"

namespace display {

char*
print_download_title(char* buf, unsigned int length, core::Download* d) {
  return buf + std::max(0, snprintf(buf, length, "%s",
				    d->get_download().get_name().c_str()));
}

char*
print_download_info(char* buf, unsigned int length, core::Download* d) {
  char* last = buf + length;

  buf += std::max(0, snprintf(buf, last - buf, "Torrent: "));

  if (!d->get_download().is_open())
    buf += std::max(0, snprintf(buf, last - buf, "closed            "));
  else if (d->is_done())
    buf += std::max(0, snprintf(buf, last - buf, "done %10.1f MB",
				(double)d->get_download().get_bytes_total() / (double)(1 << 20)));
  else
    buf += std::max(0, snprintf(buf, last - buf, "%6.1f / %6.1f MB",
				(double)d->get_download().get_bytes_done() / (double)(1 << 20),
				(double)d->get_download().get_bytes_total() / (double)(1 << 20)));
  
  buf += std::max(0, snprintf(buf, last - buf, " Rate: %5.1f / %5.1f KB Uploaded: %.1f MB",
			      (double)d->get_download().get_up_rate().rate() / (1 << 10),
			      (double)d->get_download().get_down_rate().rate() / (1 << 10),
			      (double)d->get_download().get_up_rate().total() / (1 << 20)));

  return buf;
}

char*
print_download_status(char* buf, unsigned int length, core::Download* d) {
  if (!d->get_download().is_active())
    buf += std::max(0, snprintf(buf, length, "Inactive: "));

  if (d->get_download().is_hash_checking())
    buf += std::max(0, snprintf(buf, length, "Checking hash"));

  else if (d->get_download().is_tracker_busy() &&
	   d->get_download().get_tracker_focus() < d->get_download().get_tracker_size())
    buf += std::max(0, snprintf(buf, length, "Tracker[%i:%i]: Connecting to %s",
				d->get_download().get_tracker(d->get_download().get_tracker_focus()).get_group(),
				d->get_download().get_tracker_focus(),
				d->get_download().get_tracker(d->get_download().get_tracker_focus()).get_url().c_str()));

  else if (!d->get_message().empty())
    buf += std::max(0, snprintf(buf, length, "%s", d->get_message().c_str()));

  else
    buf[0] = '\0';

  return buf;
}

std::string
print_hhmmss(utils::Timer t) {
  time_t tv_sec = static_cast<time_t>(t.tval().tv_sec);
  std::tm *u = std::localtime(&tv_sec);
  
  if (u == NULL)
    return "inv_time";

  std::stringstream str;
  str.fill('0');
  
  str << std::setw(2) << u->tm_hour << ':' << std::setw(2) << u->tm_min << ':' << std::setw(2) << u->tm_sec;

  return str.str();
}

std::string
print_ddmmyyyy(time_t t) {
  std::tm *u = std::gmtime(&t);
  
  if (u == NULL)
    return "inv_time";

  std::stringstream str;
  str.fill('0');
  
  str << std::setw(2) << u->tm_mday << '/'
      << std::setw(2) << (u->tm_mon + 1) << '/'
      << std::setw(4) << (1900 + u->tm_year);

  return str.str();
}

}
