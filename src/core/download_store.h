#ifndef RTORRENT_CORE_DOWNLOAD_STORE_H
#define RTORRENT_CORE_DOWNLOAD_STORE_H

#include <string>

#include "utils/directory.h"

namespace core {

class Download;

class DownloadStore {
public:

  void              activate(const std::string& path);
  void              disable();

  bool              is_active() { return !m_path.empty(); }

  void              save(Download* d);
  void              remove(Download* d);

  // Currently shows all entries in the correct format.
  utils::Directory  get_formated_entries();

private:
  static bool       is_correct_format(std::string f);
  std::string       create_filename(Download* d);

  std::string       m_path;
};

}

#endif
