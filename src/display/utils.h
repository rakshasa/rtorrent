#ifndef RTORRENT_DISPLAY_UTILS_H
#define RTORRENT_DISPLAY_UTILS_H

#include <string>

namespace core {
  class Download;
}

namespace utils {
  class Timer;
}

namespace display {

std::string print_download_status(core::Download* d);

std::string print_hhmmss(utils::Timer t);

}

#endif
