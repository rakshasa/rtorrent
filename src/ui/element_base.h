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

#ifndef RTORRENT_UI_ELEMENT_BASE_H
#define RTORRENT_UI_ELEMENT_BASE_H

#include "input/bindings.h"

namespace display {
  class Frame;
  class Window;
}

namespace ui {

class ElementBase {
public:
  typedef std::function<void ()> slot_type;

  ElementBase() : m_frame(NULL), m_focus(false) {}
  virtual ~ElementBase() {}

  bool                is_active() const { return m_frame != NULL; }

  input::Bindings&    bindings()        { return m_bindings; }

  virtual void        activate(display::Frame* frame, bool focus = true) = 0;
  virtual void        disable() = 0;

  void                slot_exit(const slot_type& s) { m_slot_exit = s; }

  void                mark_dirty();

protected:
  display::Frame*     m_frame;
  bool                m_focus;

  input::Bindings     m_bindings;
  slot_type           m_slot_exit;
};

}

#endif
