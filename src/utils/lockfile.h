// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

// A simple, and not guaranteed atomic, lockfile implementation. It
// saves the hostname and pid in the lock file, which may be accessed
// by Lockfile::locked_by(). If the path is an empty string then no
// lockfile will be created when Lockfile::try_lock() is called, still
// it will set the locked state of the Lockfile instance.

#ifndef RTORRENT_UTILS_LOCKFILE_H
#define RTORRENT_UTILS_LOCKFILE_H

#include <string>
#include <sys/types.h>

namespace utils {

class Lockfile {
public:
  typedef std::pair<std::string, pid_t> process_type;

  Lockfile() : m_locked(false) {}
  
  bool                is_locked() const                 { return m_locked; }
  bool                is_stale();

  // If the path is empty no lock file will be created, although
  // is_locked() will return true.
  bool                try_lock();
  bool                unlock();

  const std::string&  path() const                      { return m_path; }
  void                set_path(const std::string& path) { m_path = path; }

  std::string         locked_by_as_string() const;
  process_type        locked_by() const;

private:
  std::string         m_path;
  bool                m_locked;
};

}

#endif
