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

#ifndef RTORRENT_RPC_SCGI_TASK_H
#define RTORRENT_RPC_SCGI_TASK_H

#include <torrent/event.h>

namespace utils {
  class SocketFd;
}

namespace rpc {

class SCgi;

class SCgiTask : public torrent::Event {
public:
  static const unsigned int default_buffer_size = 2047;
  static const          int max_header_size     = 2000;
  static const          int max_content_size    = (128 << 10);

  SCgiTask() { m_fileDesc = -1; }

  bool                is_open() const      { return m_fileDesc != -1; }
  bool                is_available() const { return m_fileDesc == -1; }

  void                open(SCgi* parent, int fd);
  void                close();

  virtual void        event_read();
  virtual void        event_write();
  virtual void        event_error();

  bool                receive_write(const char* buffer, uint32_t length);

  utils::SocketFd&    get_fd()            { return *reinterpret_cast<utils::SocketFd*>(&m_fileDesc); }

private:
  SCgi*               m_parent;

  char*               m_buffer;
  char*               m_position;
  char*               m_body;

  unsigned int        m_bufferSize;
};

}

#endif
