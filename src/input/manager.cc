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

#include "config.h"

#include <algorithm>
#include <functional>
#include <stdexcept>

#include "manager.h"
#include "bindings.h"
#include "text_input.h"

namespace input {

void
Manager::erase(Bindings* b) {
  iterator itr = std::find(begin(), end(), b);

  if (itr == end())
    throw std::logic_error("Manager::erase(...) could not find binding");

  Base::erase(itr);
}

void
Manager::pressed(int key) {
  if (m_textInput != NULL && m_textInput->pressed(key))
    return;

  std::find_if(begin(), end(), std::bind2nd(std::mem_fun(&Bindings::pressed), key));
}

}
