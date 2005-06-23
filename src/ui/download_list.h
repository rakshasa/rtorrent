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

#ifndef RTORRENT_UI_DOWNLOAD_LIST_H
#define RTORRENT_UI_DOWNLOAD_LIST_H

#include <sigc++/slot.h>

#include "core/download_list.h"
#include "utils/task.h"
#include "utils/list_focus.h"

namespace input {
  class Bindings;
}

namespace display {
  class WindowDownloadList;
  class WindowHttpQueue;
  class WindowInput;
  class WindowLog;
  class WindowLogComplete;
  class WindowStatusbar;
  class WindowTitle;
}

namespace ui {

class Control;
class Download;
class ElementBase;

class DownloadList {
public:
  typedef display::WindowDownloadList              WList;
  typedef display::WindowHttpQueue                 WHttp;
  typedef display::WindowInput                     WInput;
  typedef display::WindowLog                       WLog;
  typedef display::WindowLogComplete               WLogComplete;
  typedef display::WindowStatusbar                 WStatus;
  typedef display::WindowTitle                     WTitle;

  typedef utils::ListFocus<core::DownloadList>     DList;

  typedef sigc::slot1<void, const std::string&>    SlotOpenUri;

  typedef display::Manager::iterator               MItr;

  typedef enum {
    DISPLAY_DOWNLOAD_LIST,
    DISPLAY_LOG,
    DISPLAY_STRING_LIST,
    DISPLAY_MAX_SIZE
  } Display;

  DownloadList(Control* c);
  ~DownloadList();

  input::Bindings&    get_bindings()               { return *m_bindings; }

  void                activate();
  void                disable();

  void                activate_display(Display d);
  void                disable_display();

  void                slot_open_uri(SlotOpenUri s) { m_slotOpenUri = s; }

private:
  DownloadList(const DownloadList&);
  void operator = (const DownloadList&);

  void                receive_next();
  void                receive_prev();

  void                receive_read_throttle(int t);
  void                receive_write_throttle(int t);

  void                receive_start_download();
  void                receive_stop_download();

  void                receive_view_download();
  void                receive_exit_download();

  void                receive_check_hash();

  void                receive_view_input();
  void                receive_exit_input();

  void                receive_change(Display d);

  void                task_update();

  void                setup_keys();
  void                setup_input();

  Display             m_state;

  ElementBase*        m_uiArray[DISPLAY_MAX_SIZE];

  MItr                m_window;

  WTitle*             m_windowTitle;
  WStatus*            m_windowStatus;
  WLog*               m_windowLog;
  WInput*             m_windowTextInput;
  WHttp*              m_windowHttpQueue;

  utils::TaskItem     m_taskUpdate;

  Download*           m_uiDownload;

  DList               m_downloadList;

  Control*            m_control;
  input::Bindings*    m_bindings;

  SlotOpenUri         m_slotOpenUri;
};

}

#endif
