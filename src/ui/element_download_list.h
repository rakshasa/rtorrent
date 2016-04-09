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

#ifndef RTORRENT_UI_ELEMENT_DOWNLOAD_LIST_H
#define RTORRENT_UI_ELEMENT_DOWNLOAD_LIST_H

#include "core/download_list.h"
#include "display/window_download_list.h"

#include "element_base.h"

class Control;

namespace core {
  class View;
}

namespace ui {

class ElementDownloadList : public ElementBase {
public:
  typedef display::WindowDownloadList WDownloadList;

  ElementDownloadList();

  void                activate(display::Frame* frame, bool focus = true);
  void                disable();

  core::View*         view() { return m_view; }
  WDownloadList*      window() { return m_window; }
  void                set_view(core::View* l);

  void                receive_command(const char* cmd);

  void                receive_next();
  void                receive_prev();

  void                receive_stop_download();
  void                receive_close_download();

  void                receive_next_priority();
  void                receive_prev_priority();

  void                receive_cycle_throttle();

  void                receive_change_view(const std::string& name);

private:
  WDownloadList*      m_window;
  core::View*         m_view;
};

}

#endif
