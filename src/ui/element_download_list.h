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

#ifndef RTORRENT_UI_ELEMENT_DOWNLOAD_LIST_H
#define RTORRENT_UI_ELEMENT_DOWNLOAD_LIST_H

#include "core/download_list.h"
#include "utils/list_focus.h"

#include "element_base.h"

namespace display {
  class WindowDownloadList;
}

namespace ui {

class Control;

class ElementDownloadList : public ElementBase {
public:
  typedef display::WindowDownloadList          WDownloadList;
  typedef utils::ListFocus<core::DownloadList> DList;

  ElementDownloadList(DList* l);

  void                activate(Control* c, MItr mItr);
  void                disable(Control* c);

private:
  WDownloadList*      m_window;
  
  DList*              m_list;
};

}

#endif
