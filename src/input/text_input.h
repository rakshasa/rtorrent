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

#ifndef RTORRENT_INPUT_TEXT_INPUT_H
#define RTORRENT_INPUT_TEXT_INPUT_H

#include <string>
#include <sigc++/slot.h>

namespace input {

class TextInput : private std::string {
public:
  typedef std::string       Base;
  typedef sigc::slot0<void> SlotDirty;

  using Base::c_str;
  using Base::empty;
  using Base::size;

  TextInput() : m_pos(0), m_alt(false) {}

  size_type get_pos()                  { return m_pos; }

  bool      pressed(int key);

  void      clear()                    { m_pos = 0; m_alt = false; Base::clear(); }

  void      slot_dirty(SlotDirty s)    { m_slotDirty = s; }

  const std::string& str()             { return *this; }

private:
  size_type m_pos;

  bool      m_alt;
  SlotDirty m_slotDirty;
};

}

#endif
