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

#include "config.h"

#include <cctype>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lockfile.h"

namespace utils {

bool
Lockfile::try_lock() {
  if (m_path.empty()) {
    m_locked = true;
    return true;
  }

  // Just do a simple locking for now that isn't safe for network
  // devices.
  int fd = ::open(m_path.c_str(), O_RDWR | O_CREAT | O_EXCL, 0444);

  if (fd == -1)
    return false;

  char buf[256];
  int pos = ::gethostname(buf, 255);

  if (pos == 0) {
    ::snprintf(buf + std::strlen(buf), 255, ":+%i\n", ::getpid());
    ::write(fd, buf, std::strlen(buf));
  }

  ::close(fd);

  m_locked = true;
  return true;
}

bool
Lockfile::unlock() {
  m_locked = false;

  if (m_path.empty())
    return true;
  else
    return ::unlink(m_path.c_str()) != -1;
}

struct lockfile_valid_char {
  bool operator () (char c) {
    return !std::isgraph(c);
  }
};

std::string
Lockfile::locked_by() const {
  int fd = ::open(m_path.c_str(), O_RDONLY);
  
  if (fd < 0)
    return "<error>";

  char buf[256];
  int pos = read(fd, buf, 255);

  ::close(fd);

  return std::string(buf, std::find_if(buf, buf + std::max(pos, 0), lockfile_valid_char()));
}

}
