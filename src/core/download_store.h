#ifndef RTORRENT_CORE_DOWNLOAD_STORE_H
#define RTORRENT_CORE_DOWNLOAD_STORE_H

#include <string>

#include "utils/lockfile.h"

namespace utils {
  class Directory;
}

namespace core {

class Download;

class DownloadStore {
public:
  static const int flag_skip_static = 0x1;

  bool                is_enabled()                            { return m_lockfile.is_locked(); }

  void                enable(bool lock);
  void                disable();

  const std::string&  path() const                            { return m_path; }
  void                set_path(const std::string& path);

  bool                save(Download* d, int flags);
  bool                save_full(Download* d)   { return save(d, 0); }
  bool                save_resume(Download* d) { return save(d, flag_skip_static); }
  void                remove(Download* d);

  // Currently shows all entries in the correct format.
  utils::Directory    get_formated_entries();

  static bool         is_correct_format(const std::string& f);

private:
  std::string         create_filename(Download* d);

  bool                write_bencode(const std::string& filename, const torrent::Object& obj, uint32_t skip_mask);

  std::string         m_path;
  utils::Lockfile     m_lockfile;
};

}

#endif
