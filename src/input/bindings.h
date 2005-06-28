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

#ifndef RTORRENT_INPUT_BINDINGS_H
#define RTORRENT_INPUT_BINDINGS_H

#include <map>
#include <ncurses.h>
#include <sigc++/slot.h>

namespace input {

class Bindings : private std::map<int, sigc::slot0<void> > {
public:
  typedef sigc::slot0<void>    Slot;
  typedef std::map<int, Slot > Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;
  using Base::find;

  using Base::erase;

  using Base::operator[];

  Bindings() : m_active(true) {}

  void                activate()          { m_active = true; }
  void                disable()           { m_active = false; }

  bool                pressed(int key);

  void                ignore(int key)     { (*this)[key] = Slot(); }

private:
  bool                m_active;
};

}

#endif
