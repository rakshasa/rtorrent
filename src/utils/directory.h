// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_UTILS_DIRECTORY_H
#define RTORRENT_UTILS_DIRECTORY_H

#include <string>
#include <vector>
#include <inttypes.h>

namespace utils {

struct directory_entry {
  // Fix.
  bool is_file() const { return true; }

  // The name and types should match POSIX.
  uint32_t            d_fileno;
  uint32_t            d_reclen;
  uint8_t             d_type;

  std::string         d_name;
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

  Directory() {}
  Directory(const std::string& path) : m_path(path) {}

  bool                is_valid() const;

  const std::string&  path()                            { return m_path; }
  void                set_path(const std::string& path) { m_path = path; }

  bool                update(int flags);

private:
  std::string         m_path;
};

inline bool operator == (const directory_entry& left, const directory_entry& right) { return left.d_name == right.d_name; }
inline bool operator != (const directory_entry& left, const directory_entry& right) { return left.d_name != right.d_name; }
inline bool operator <  (const directory_entry& left, const directory_entry& right) { return left.d_name <  right.d_name; }
inline bool operator >  (const directory_entry& left, const directory_entry& right) { return left.d_name >  right.d_name; }
inline bool operator <= (const directory_entry& left, const directory_entry& right) { return left.d_name <= right.d_name; }
inline bool operator >= (const directory_entry& left, const directory_entry& right) { return left.d_name >= right.d_name; }

}

#endif
