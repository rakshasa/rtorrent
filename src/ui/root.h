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

#ifndef RTORRENT_UI_ROOT_H
#define RTORRENT_UI_ROOT_H

#include <inttypes.h>
#include "input/bindings.h"

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

class Root {
public:
  typedef display::WindowTitle     WTitle;
  typedef display::WindowHttpQueue WHttpQueue;
  typedef display::WindowInput     WInput;
  typedef display::WindowStatusbar WStatusbar;

  Root();

  void                init(Control* c);
  void                cleanup();

  WTitle*             window_title()                          { return m_windowTitle; }
  WStatusbar*         window_statusbar()                      { return m_windowStatusbar; }
  WInput*             window_input()                          { return m_windowInput; }

  void                set_down_throttle(unsigned int throttle);
  void                set_up_throttle(unsigned int throttle);

  // Rename to raw or something, make base function.
  void                set_down_throttle_i64(int64_t throttle) { set_down_throttle(throttle >> 10); }
  void                set_up_throttle_i64(int64_t throttle)   { set_up_throttle(throttle >> 10); }

  void                adjust_down_throttle(int throttle);
  void                adjust_up_throttle(int throttle);

  unsigned int        max_uploads_global()                    { return m_maxUploadsGlobal; }
  void                set_max_uploads_global(int64_t slots);

  unsigned int        max_downloads_global()                  { return m_maxDownloadsGlobal; }
  void                set_max_downloads_global(int64_t slots);

  void                enable_input(const std::string& title, input::TextInput* input);
  void                disable_input();

  input::TextInput*   current_input();

private:
  void                setup_keys();

  Control*            m_control;
  DownloadList*       m_downloadList;

  WTitle*             m_windowTitle;
  WHttpQueue*         m_windowHttpQueue;
  WInput*             m_windowInput;
  WStatusbar*         m_windowStatusbar;

  input::Bindings     m_bindings;

  unsigned int        m_maxUploadsGlobal;
  unsigned int        m_maxDownloadsGlobal;
};

}

#endif
