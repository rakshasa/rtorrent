// libTorrent - BitTorrent library
// Copyright (C) 2005, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_UTILS_FILE_STAT_H
#define RTORRENT_UTILS_FILE_STAT_H

#include <string>
#include <inttypes.h>
#include <sys/stat.h>

namespace utils {

class FileStat {
public:
  FileStat() {}
  FileStat(const char* filename)                            { update_throws(filename); }
  explicit FileStat(int fd)                                 { update_throws(fd); }

  int                 update(int fd)                        { return fstat(fd, &m_stat); }
  int                 update(const char* filename)          { return stat(filename, &m_stat); }

  void                update_throws(int fd);
  void                update_throws(const char* filename);

  bool                is_regular() const                    { return S_ISREG(m_stat.st_mode); }
  bool                is_directory() const                  { return S_ISDIR(m_stat.st_mode); }
  bool                is_character() const                  { return S_ISCHR(m_stat.st_mode); }
  bool                is_block() const                      { return S_ISBLK(m_stat.st_mode); }
  bool                is_fifo() const                       { return S_ISFIFO(m_stat.st_mode); }
  bool                is_link() const                       { return S_ISLNK(m_stat.st_mode); }
  bool                is_sockt() const                      { return S_ISSOCK(m_stat.st_mode); }

  off_t               get_size() const                      { return m_stat.st_size; }

  time_t              get_atime() const                     { return m_stat.st_atime; }
  time_t              get_ctime() const                     { return m_stat.st_ctime; }
  time_t              get_mtime() const                     { return m_stat.st_mtime; }

  static std::string  error_string(int err);

private:
  struct stat         m_stat;
};

}

#endif
