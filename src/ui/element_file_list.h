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

#ifndef RTORRENT_UI_ELEMENT_FILE_LIST_H
#define RTORRENT_UI_ELEMENT_FILE_LIST_H

#include <torrent/common.h>
#include <torrent/data/file_list_iterator.h>

#include "core/download.h"

#include "element_base.h"

class Control;

namespace display {
  class WindowFileList;
}

namespace ui {

class ElementText;

class ElementFileList : public ElementBase {
public:
  typedef torrent::priority_t Priority;
  typedef display::WindowFileList  WFileList;

  typedef torrent::FileListIterator iterator;

  typedef enum {
    DISPLAY_LIST,
    DISPLAY_INFO,
    DISPLAY_MAX_SIZE
  } Display;

  ElementFileList(core::Download* d);

  void                activate(display::Frame* frame, bool focus = true);
  void                disable();

  void                activate_display(Display display);

private:
  void                receive_next();
  void                receive_prev();
  void                receive_pagenext();
  void                receive_pageprev();

  void                receive_priority();
  void                receive_change_all();

  Priority            next_priority(Priority p);

  void                update_itr();

  core::Download*     m_download;

  Display             m_state;
  WFileList*          m_window;
  ElementText*        m_elementInfo;
  
  // Change to unsigned, please.
  iterator            m_selected;
};

}

#endif
