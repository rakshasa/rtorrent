// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#ifndef RTORRENT_RPC_SCGI_H
#define RTORRENT_RPC_SCGI_H

#include <string>
#include <rak/functional_fun.h>
#include <torrent/event.h>

#include "scgi_task.h"

namespace utils {
  class SocketFd;
}

namespace rpc {

class lt_cacheline_aligned SCgi : public torrent::Event {
public:
  typedef rak::function2<bool, const char*, uint32_t>             slot_write;

  static const int max_tasks = 100;

  // Global lock:
  SCgi() : m_logFd(-1) {}
  virtual ~SCgi();

  const char*         type_name() const { return "scgi"; }

  void                open_port(void* sa, unsigned int length, bool dontRoute);
  void                open_named(const std::string& filename);

  void                activate();
  void                deactivate();

  const std::string&  path() const { return m_path; }

  int                 log_fd() const     { return m_logFd; }
  void                set_log_fd(int fd) { m_logFd = fd; }

  // Thread local:
  virtual void        event_read();
  virtual void        event_write();
  virtual void        event_error();

  bool                receive_call(SCgiTask* task, const char* buffer, uint32_t length);

  utils::SocketFd&    get_fd()            { return *reinterpret_cast<utils::SocketFd*>(&m_fileDesc); }

private:
  void                open(void* sa, unsigned int length);

  std::string         m_path;
  int                 m_logFd;
  SCgiTask            m_task[max_tasks];
};

}

#endif
