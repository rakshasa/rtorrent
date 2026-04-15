#ifndef RTORRENT_UTILS_DIRECTORY_H
#define RTORRENT_UTILS_DIRECTORY_H

#include <cstdint>
#include <string>
#include <vector>

namespace utils {

struct directory_entry {
  // Fix.
  bool is_file() const { return true; }

  // The name and types should match POSIX.
  uint32_t            s_fileno;
  uint32_t            s_reclen; //Not used. Messes with Solaris.
  uint8_t             s_type;

  std::string         s_name;
};

class Directory : private std::vector<directory_entry> {
public:
  typedef std::vector<directory_entry> base_type;

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

  static const uint32_t buffer_size = 64 * 1024;

  static const int update_sort     = 0x1;
  static const int update_hide_dot = 0x2;

  Directory() = default;
  Directory(const std::string& path) : m_path(path) {}

  bool                is_valid() const;

  const std::string&  path() const                      { return m_path; }
  void                set_path(const std::string& path) { m_path = path; }

  bool                update(int flags);

private:
  std::string         m_path;
};

inline bool operator == (const directory_entry& left, const directory_entry& right) { return left.s_name == right.s_name; }
inline bool operator != (const directory_entry& left, const directory_entry& right) { return left.s_name != right.s_name; }
inline bool operator <  (const directory_entry& left, const directory_entry& right) { return left.s_name <  right.s_name; }
inline bool operator >  (const directory_entry& left, const directory_entry& right) { return left.s_name >  right.s_name; }
inline bool operator <= (const directory_entry& left, const directory_entry& right) { return left.s_name <= right.s_name; }
inline bool operator >= (const directory_entry& left, const directory_entry& right) { return left.s_name >= right.s_name; }

}

#endif
