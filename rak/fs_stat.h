// rak - Rakshasa's toolbox
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

#ifndef RAK_FS_STAT_H
#define RAK_FS_STAT_H

#include <string>
#include <inttypes.h>

#include <rak/error_number.h>

#if HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#if HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

namespace rak {

class fs_stat {
public:

  typedef FS_STAT_SIZE_TYPE  blocksize_type;
  typedef FS_STAT_COUNT_TYPE blockcount_type;
  typedef FS_STAT_STRUCT     fs_stat_type;

  bool                       update(int fd)                       { return FS_STAT_FD; }
  bool                       update(const char* fn)               { return FS_STAT_FN; }
  bool                       update(const std::string& filename)  { return update(filename.c_str()); }

  blocksize_type             blocksize()                          { return FS_STAT_BLOCK_SIZE; }
  blockcount_type            blocks_avail()                       { return m_stat.f_bavail; }
  int64_t                    bytes_avail()                        { return (int64_t) blocksize() * m_stat.f_bavail; }

private:
  fs_stat_type               m_stat;
};

}

#endif
