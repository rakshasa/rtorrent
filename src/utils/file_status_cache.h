#ifndef RTORRENT_UTILS_FILE_STATUS_CACHE_H
#define RTORRENT_UTILS_FILE_STATUS_CACHE_H

#include <map>
#include <string>
#include <cinttypes>

namespace utils {

struct file_status {
  int      m_flags;
  uint32_t m_mtime;
};

class FileStatusCache : public std::map<std::string, file_status> {
public:
  typedef std::map<std::string, file_status> base_type;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;
  using base_type::value_type;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::empty;
  using base_type::size;

  using base_type::erase;

  // Insert and return true if the entry does not exist or the new
  // file's mtime is more recent.
  bool                insert(const std::string& path);

  // Add a function for pruning a sorted list of paths.

  // Function for pruning entries that no longer points to a file, or
  // has a different mtime.
  void                prune();
};

}

#endif
