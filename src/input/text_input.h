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

#ifndef RTORRENT_INPUT_TEXT_INPUT_H
#define RTORRENT_INPUT_TEXT_INPUT_H

#include <string>

#include "bindings.h"

namespace input {

class TextInput : private std::string {
public:
  typedef std::string                 Base;
  typedef std::function<void ()> slot_void;

  using Base::c_str;
  using Base::empty;
  using Base::size;
  using Base::size_type;
  using Base::npos;

  TextInput() : m_pos(0), m_alt(false) {}
  virtual ~TextInput() {}

  size_type           get_pos()                  { return m_pos; }
  void                set_pos(size_type pos)     { m_pos = pos; }

  virtual bool        pressed(int key);

  void                clear()                    { m_pos = 0; m_alt = false; Base::clear(); }

  void                slot_dirty(slot_void s)    { m_slot_dirty = s; }
  void                mark_dirty()               { if (m_slot_dirty) m_slot_dirty(); }

  std::string&        str()                      { return *this; }

  Bindings&           bindings()                 { return m_bindings; }

private:
  size_type           m_pos;

  bool                m_alt;
  slot_void           m_slot_dirty;

  Bindings            m_bindings;
};

}

#endif
