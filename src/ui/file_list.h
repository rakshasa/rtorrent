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

#ifndef RTORRENT_UI_FILE_LIST_H
#define RTORRENT_UI_FILE_LIST_H

#include "core/download.h"
#include "display/manager.h"
#include "input/bindings.h"

namespace display {
  class WindowFileList;
}

namespace ui {

class Control;

class FileList {
public:
  typedef display::WindowFileList    WFileList;
  typedef display::Manager::iterator MItr;

  FileList(Control* c, core::Download* d);

  void                activate(MItr mItr);
  void                disable();

  input::Bindings&    get_bindings() { return m_bindings; }

private:
  void                receive_next();
  void                receive_prev();

  void                receive_priority();

  core::Download*     m_download;
  WFileList*          m_window;
  
  Control*            m_control;
  input::Bindings     m_bindings;

  // Change to unsigned, please.
  unsigned int        m_focus;
};

}

#endif
