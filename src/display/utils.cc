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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <sstream>
#include <iomanip>

#include "core/download.h"
#include "utils/timer.h"

#include "utils.h"

namespace display {

std::string
print_download_status(core::Download* d) {
  if (d->get_download().is_hash_checking()) {
    return "Checking hash";

  } else if (d->get_download().is_tracker_busy()) {
    return "Tracker: Connecting";

  } else if (!d->get_download().is_active()) {
    return "Inactive";

  } else if (!d->get_message().empty()) {
    return d->get_message();

  } else {
    return "";
  }
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
      << std::setw(2) << u->tm_mon << '/'
      << std::setw(4) << (1900 + u->tm_year);

  return str.str();
}

}
