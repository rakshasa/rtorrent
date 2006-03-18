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

#ifndef RTORRENT_UI_DOWNLOAD_LIST_H
#define RTORRENT_UI_DOWNLOAD_LIST_H

#include <sigc++/slot.h>

#include "core/download_list.h"
#include "display/manager.h"
#include "utils/list_focus.h"

#include "globals.h"

class Control;

namespace input {
  class Bindings;
}

namespace display {
  class WindowDownloadList;
  class WindowHttpQueue;
  class WindowInput;
  class WindowLog;
  class WindowLogComplete;
  class WindowTitle;
}

namespace ui {

class Download;
class ElementBase;

class DownloadList {
public:
  typedef display::WindowDownloadList              WList;
  typedef display::WindowHttpQueue                 WHttp;
  typedef display::WindowInput                     WInput;
  typedef display::WindowLog                       WLog;
  typedef display::WindowLogComplete               WLogComplete;
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

  typedef enum {
    INPUT_NONE,
    INPUT_LOAD_DEFAULT,
    INPUT_LOAD_MODIFIED,
    INPUT_CHANGE_DIRECTORY,
    INPUT_COMMAND
  } Input;

  DownloadList(Control* c);
  ~DownloadList();

  input::Bindings&    get_bindings()               { return *m_bindings; }

  bool                is_active() const            { return m_window != m_control->display()->end(); }

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

  void                receive_start_download();
  void                receive_stop_download();
  void                receive_close_download();

  void                receive_view_download();
  void                receive_exit_download();

  void                receive_next_priority();
  void                receive_prev_priority();

  void                receive_check_hash();

  void                receive_view_input(Input type);
  void                receive_exit_input(Input type);

  void                receive_change(Display d);

  void                receive_download_erased(core::Download* d);

  void                task_update();

  void                setup_keys();
  void                setup_input();

  Display             m_state;

  ElementBase*        m_uiArray[DISPLAY_MAX_SIZE];

  MItr                m_window;

  WTitle*             m_windowTitle;
  WLog*               m_windowLog;
  WInput*             m_windowTextInput;
  WHttp*              m_windowHttpQueue;

  rak::priority_item  m_taskUpdate;

  Download*           m_uiDownload;

  DList               m_downloadList;

  Control*            m_control;
  input::Bindings*    m_bindings;

  SlotOpenUri         m_slotOpenUri;
};

}

#endif
