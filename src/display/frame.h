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

#ifndef RTORRENT_DISPLAY_FRAME_H
#define RTORRENT_DISPLAY_FRAME_H

#include <inttypes.h>

namespace display {

class Window;

class Frame {
public:
  typedef uint32_t                       extent_type;
  typedef uint32_t                       size_type;

  enum Type {
    TYPE_NONE,
    TYPE_WINDOW,
    TYPE_ROW,
    TYPE_COLUMN
  };

  struct bounds_type {
    bounds_type() {}
    bounds_type(extent_type minW, extent_type minH, extent_type maxW, extent_type maxH) :
      minWidth(minW), minHeight(minH), maxWidth(maxW), maxHeight(maxH) {}

    extent_type minWidth;
    extent_type minHeight;

    extent_type maxWidth;
    extent_type maxHeight;
  };

  typedef std::pair<Frame*, bounds_type> dynamic_type;

  static const size_type max_size = 5;

  Frame();

  bool                is_width_dynamic() const;
  bool                is_height_dynamic() const;

  bool                has_left_frame() const;
  bool                has_bottom_frame() const;

  bounds_type         preferred_size() const;

  uint32_t            position_x() const                { return m_positionX; }
  uint32_t            position_y() const                { return m_positionY; }

  uint32_t            width() const                     { return m_width; }
  uint32_t            height() const                    { return m_height; }

  Type                get_type() const                  { return m_type; }
  void                set_type(Type t);

  Window*             window() const                    { return m_window; }

  Frame*              frame(size_type idx)              { return m_container[idx]; }

  size_type           container_size() const            { return m_containerSize; }
  void                set_container_size(size_type size);

  void                initialize_window(Window* window);
  void                initialize_row(size_type size);
  void                initialize_column(size_type size);

  void                clear();

  void                refresh();
  void                redraw();

  void                balance(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

private:
  inline void         balance_window(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
  inline void         balance_row(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
  inline void         balance_column(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

  Type                m_type;

  uint32_t            m_positionX;
  uint32_t            m_positionY;
  uint32_t            m_width;
  uint32_t            m_height;

  union {
    Window*             m_window;
    
    struct {
      size_type           m_containerSize;
      Frame*              m_container[max_size];
    };
  };
};

}

#endif
