#include "config.h"

#include <ctime>
#include <sstream>

#include "core/download.h"
#include "utils/timer.h"

#include "utils.h"

namespace display {

std::string
print_download_status(core::Download* d) {
  std::stringstream str;

  if (d->get_download().is_hash_checking()) {
    str << "Checking hash";

  } else if (d->get_download().is_tracker_busy()) {
    str << "Tracker: Connecting";

  } else if (!d->get_download().is_active()) {
    str << "Inactive";

  } else if (!d->get_tracker_msg().empty()) {
    str << "Tracker: [" << d->get_tracker_msg() << ']';

  } else {
    //str << "---";
  }

  return str.str();
}

std::string
print_hhmmss(utils::Timer t) {
  std::tm *u = std::localtime(&t.tval().tv_sec);
  
  if (u == NULL)
    return "inv_time";

  std::stringstream str;
  str.width(2);
  str.fill('0');
  
  str << u->tm_hour << ':' << u->tm_min << ':' << u->tm_sec;

  return str.str();
}

}
