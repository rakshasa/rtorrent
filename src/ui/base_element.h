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

#ifndef RTORRENT_UI_BASE_ELEMENT_H
#define RTORRENT_UI_BASE_ELEMENT_H

#include "display/manager.h"
#include "input/bindings.h"

namespace ui {

class BaseElement {
public:
  typedef display::Manager::iterator MItr;

  virtual ~BaseElement() {}

  virtual void        activate(Control* c, MItr mItr) = 0;
  virtual void        disable(Control* c) = 0;

  input::Bindings&    get_bindings() { return m_bindings; }

protected:
  input::Bindings     m_bindings;
};

}

#endif
