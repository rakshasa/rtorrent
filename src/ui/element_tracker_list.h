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

#ifndef RTORRENT_UI_ELEMENT_TRACKER_LIST_H
#define RTORRENT_UI_ELEMENT_TRACKER_LIST_H

#include "core/download.h"

#include "element_base.h"

class Control;

namespace display {
  class WindowTrackerList;
}

namespace ui {

class ElementTrackerList : public ElementBase {
public:
  typedef display::WindowTrackerList    WTrackerList;

  ElementTrackerList(core::Download* d);

  void                activate(display::Frame* frame);
  void                disable();

  display::Window*    window();

private:
  void                receive_next();
  void                receive_prev();

  void                receive_disable();

  void                receive_cycle_group();

  core::Download*     m_download;
  WTrackerList*       m_window;
  
  // Change to unsigned, please.
  unsigned int        m_focus;
};

}

#endif
