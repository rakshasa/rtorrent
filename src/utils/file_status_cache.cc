#include "config.h"

#include "file_status_cache.h"

#include <filesystem>
#include <rak/path.h>
#include <sys/stat.h>
#include <torrent/exceptions.h>

namespace utils {

bool
FileStatusCache::insert(const std::string& path) {

  // Should we expand somewhere else? Problem is it adds a lot of junk
  // to the start of the paths added to the cache, causing more work
  // during search, etc.

  // Return false if the file hasn't been modified since last time. We
  // use 'equal to' instead of 'greater than' since the file might
  // have been replaced by another file, and thus should be re-tried.


  // TODO: The below code requires C++20 for std::chrono::clock_cast.
  //
  // std::error_code ec;
  //
  // auto mtime_real = std::filesystem::last_write_time(rak::path_expand(path), ec);
  //
  // if (ec)
  //   return false;
  //
  // auto mtime_system  = std::chrono::clock_cast<std::chrono::system_clock>(mtime_real);
  // auto mtime_seconds = std::chrono::duration_cast<std::chrono::seconds>(mtime_system.time_since_epoch()).count();
  //
  // if (mtime_seconds > UINT32_MAX)
  //   throw torrent::internal_error("FileStatusCache::insert() file mtime exceeds uint32_t");

  struct stat fs;

  if (stat(rak::path_expand(path).c_str(), &fs) != 0)
    return false;

  if (fs.st_mtime < 0)
    throw torrent::internal_error("FileStatusCache::insert() file mtime is negative");

  int64_t mtime_seconds = fs.st_mtime;

  std::pair<iterator, bool> result = base_type::insert(value_type(path, file_status()));

  if (!result.second && result.first->second.m_mtime == mtime_seconds)
    return false;

  result.first->second.m_flags = 0;
  result.first->second.m_mtime = mtime_seconds;

  return true;
}

void
FileStatusCache::prune() {
  iterator itr = begin();

  while (itr != end()) {
    iterator tmp = itr++;

    struct stat fs;

    if (stat(rak::path_expand(tmp->first).c_str(), &fs) != 0) {
      base_type::erase(tmp);
      continue;
    }

    if (fs.st_mtime < 0)
      throw torrent::internal_error("FileStatusCache::prune() file mtime is negative");

    if (static_cast<int64_t>(fs.st_mtime) != tmp->second.m_mtime)
      base_type::erase(tmp);
  }
}

}
