#ifndef RTORRENT_CORE_DOWNLOAD_STORE_H
#define RTORRENT_CORE_DOWNLOAD_STORE_H

#include <string>
#include <torrent/common.h>

namespace utils {
  class Directory;
}

namespace core {

class Download;

class DownloadStore {
public:
  // TODO: Move to session manager.

  // Currently shows all entries in the correct format.
  utils::Directory    get_formated_entries();

  static bool         is_correct_format(const std::string& f);
};

}

#endif
