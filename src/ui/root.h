// rTorrent - BitTorrent client
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

#ifndef RTORRENT_UI_ROOT_H
#define RTORRENT_UI_ROOT_H

#include "input/bindings.h"

namespace display {
  class WindowStatusbar;
}

namespace ui {

class DownloadList;

class Root {
public:
  typedef display::WindowStatusbar WStatus;

  Root(Control* c);

  void                init();
  void                cleanup();

  bool                get_shutdown_received()       { return m_shutdownReceived; }
  void                set_shutdown_received(bool v) { m_shutdownReceived = v; }

private:
  void                setup_keys();

  void                receive_read_throttle(int t);
  void                receive_write_throttle(int t);

  bool                m_shutdownReceived;

  Control*            m_control;
  DownloadList*       m_downloadList;

  WStatus*            m_windowStatus;

  input::Bindings     m_bindings;
};

}

#endif
