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

#ifndef RTORRENT_UI_CONTROL_H
#define RTORRENT_UI_CONTROL_H

#include "core/manager.h"
#include "display/manager.h"
#include "input/manager.h"

#include "root.h"

namespace ui {

class Control {
public:
  Control() : m_shutdownReceived(false) {}
  
  bool                get_shutdown_received()       { return m_shutdownReceived; }
  void                set_shutdown_received(bool v) { m_shutdownReceived = v; }

  Root&               get_ui()                      { return m_ui; }
  core::Manager&      get_core()                    { return m_core; }
  display::Manager&   get_display()                 { return m_display; }
  input::Manager&     get_input()                   { return m_input; }

private:
  Control(const Control&);
  void operator = (const Control&);

  bool                m_shutdownReceived;

  Root                m_ui;
  core::Manager       m_core;
  display::Manager    m_display;
  input::Manager      m_input;
};

}

#endif
