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

#ifndef RTORRENT_UI_ROOT_H
#define RTORRENT_UI_ROOT_H

#include <cstdint>
#include "input/bindings.h"
#include "download_list.h"

class Control;

namespace display {
  class Frame;
  class WindowTitle;
  class WindowHttpQueue;
  class WindowInput;
  class WindowStatusbar;
}

namespace input {
  class TextInput;
}

namespace ui {

class DownloadList;

typedef std::vector<std::string> ThrottleNameList;

class Root {
public:
  typedef display::WindowTitle     WTitle;
  typedef display::WindowHttpQueue WHttpQueue;
  typedef display::WindowInput     WInput;
  typedef display::WindowStatusbar WStatusbar;

  typedef std::map<int, int> InputHistoryPointers;
  typedef std::vector<std::string> InputHistoryCategory;
  typedef std::map<int, InputHistoryCategory> InputHistory;

  Root();

  void                init(Control* c);
  void                cleanup();

  WTitle*             window_title()                          { return m_windowTitle; }
  WStatusbar*         window_statusbar()                      { return m_windowStatusbar; }
  WInput*             window_input()                          { return m_windowInput; }

  DownloadList*       download_list()                         { return m_downloadList; }

  void                set_down_throttle(unsigned int throttle);
  void                set_up_throttle(unsigned int throttle);

  // Rename to raw or something, make base function.
  void                set_down_throttle_i64(int64_t throttle) { set_down_throttle(throttle >> 10); }
  void                set_up_throttle_i64(int64_t throttle)   { set_up_throttle(throttle >> 10); }

  void                adjust_down_throttle(int throttle);
  void                adjust_up_throttle(int throttle);

  const char*         get_throttle_keys();

  ThrottleNameList&   get_status_throttle_up_names()          { return m_throttle_up_names; }
  ThrottleNameList&   get_status_throttle_down_names()        { return m_throttle_down_names; }

  void                set_status_throttle_up_names(const ThrottleNameList& throttle_list)      { m_throttle_up_names = throttle_list; }
  void                set_status_throttle_down_names(const ThrottleNameList& throttle_list)    { m_throttle_down_names = throttle_list; }

  void                enable_input(const std::string& title, input::TextInput* input, ui::DownloadList::Input type);
  void                disable_input();

  input::TextInput*   current_input();

  int                 get_input_history_size()                { return m_input_history_length; }
  void                set_input_history_size(int size);
  void                add_to_input_history(ui::DownloadList::Input type, std::string item);

  void                load_input_history();
  void                save_input_history();
  void                clear_input_history();

private:
  void                setup_keys();

  Control*            m_control;
  DownloadList*       m_downloadList;

  WTitle*             m_windowTitle;
  WHttpQueue*         m_windowHttpQueue;
  WInput*             m_windowInput;
  WStatusbar*         m_windowStatusbar;

  input::Bindings     m_bindings;

  int                   m_input_history_length;
  std::string           m_input_history_last_input;
  int                   m_input_history_pointer_get;
  InputHistory          m_input_history;
  InputHistoryPointers  m_input_history_pointers;

  void                prev_in_input_history(ui::DownloadList::Input type);
  void                next_in_input_history(ui::DownloadList::Input type);

  void                reset_input_history_attributes(ui::DownloadList::Input type);

  ThrottleNameList    m_throttle_up_names;
  ThrottleNameList    m_throttle_down_names;
};

}

#endif
