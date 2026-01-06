#include "config.h"

#include "utils/file_status_cache.h"

#include <torrent/exceptions.h>
#include <torrent/utils/file_stat.h>

#include "globals.h"

namespace utils {

bool
FileStatusCache::insert(const std::string& path) {
  torrent::utils::FileStat fs;

  // Should we expand somewhere else? Problem is it adds a lot of junk
  // to the start of the paths added to the cache, causing more work
  // during search, etc.
  if (!fs.update(expand_path(path)))
    return false;

  std::pair<iterator, bool> result = base_type::insert(value_type(path, file_status()));

  // Return false if the file hasn't been modified since last time. We
  // use 'equal to' instead of 'greater than' since the file might
  // have been replaced by another file, and thus should be re-tried.
  if (!result.second && result.first->second.m_mtime == (uint32_t)fs.modified_time())
    return false;

  result.first->second.m_flags = 0;
  result.first->second.m_mtime = fs.modified_time();

  return true;
}

void
FileStatusCache::prune() {
  iterator itr = begin();

  while (itr != end()) {
    torrent::utils::FileStat fs;
    iterator tmp = itr++;

    if (!fs.update(expand_path(tmp->first)) || tmp->second.m_mtime != (uint32_t)fs.modified_time())
      base_type::erase(tmp);
  }
}

}
