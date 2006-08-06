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

#ifndef RTORRENT_DISPLAY_WINDOW_INPUT_H
#define RTORRENT_DISPLAY_WINDOW_INPUT_H

#include <string>

#include "window.h"

namespace input {
  class TextInput;
}

namespace display {

class WindowInput : public Window {
public:
  WindowInput() :
    Window(new Canvas, 0, 0, 1, extent_full, 1),
    m_input(NULL),
    m_focus(false) {}

  input::TextInput*   input()                            { return m_input; }
  void                set_input(input::TextInput* input) { m_input = input; }

  const std::string&  title() const                      { return m_title; }
  void                set_title(const std::string& str)  { m_title = str; }

  bool                focus() const                      { return m_focus; }
  void                set_focus(bool f)                  { m_focus = f; if (is_active()) mark_dirty(); }

  virtual void        redraw();

private:
  input::TextInput*   m_input;
  std::string         m_title;

  bool                m_focus;
};

}

#endif
