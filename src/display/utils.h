#ifndef RTORRENT_DISPLAY_UTILS_H
#define RTORRENT_DISPLAY_UTILS_H

#include <string>

namespace core {
  class Download;
}

namespace display {

std::string print_download_status(core::Download* d);

}

#endif
