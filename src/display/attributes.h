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

#ifndef RTORRENT_DISPLAY_ATTRIBUTES_H
#define RTORRENT_DISPLAY_ATTRIBUTES_H

#include <string>
#include <vector>
#include <ncurses.h>

// Let us hail the creators of curses for being idiots. The only
// clever move they made was in the naming.
#undef timeout
#undef move

namespace display {

class Attributes {
public:
  static const int a_invalid = ~int();
  static const int a_normal  = A_NORMAL;
  static const int a_bold    = A_BOLD;
  static const int a_reverse = A_REVERSE;

  static const int color_invalid = ~int();
  static const int color_default = 0;

  Attributes() {}
  Attributes(const char* pos, int attr, int col) :
    m_position(pos), m_attributes(attr), m_colors(col) {}

  const char*         position() const              { return m_position; }
  void                set_position(const char* pos) { m_position = pos; }

  int                 attributes() const            { return m_attributes; }
  void                set_attributes(int attr)      { m_attributes = attr; }

  int                 colors() const                { return m_colors; }
  void                set_colors(int col)           { m_colors = col; }

private:
  const char*         m_position;
  int                 m_attributes;
  int                 m_colors;
};

}

#endif
