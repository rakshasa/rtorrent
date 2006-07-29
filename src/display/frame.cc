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

#include "config.h"

#include <algorithm>
#include <functional>
#include <rak/algorithm.h>
#include <torrent/exceptions.h>

#include "frame.h"
#include "window.h"

namespace display {

Frame::Frame() :
  m_type(TYPE_NONE),

  m_positionX(0),
  m_positionY(0),
  m_width(0),
  m_height(0) {
}

bool
Frame::is_width_dynamic() const {
  switch (m_type) {
  case TYPE_NONE:
    return false;

  case TYPE_WINDOW:
    return m_window->is_active() && m_window->is_width_dynamic();

  case TYPE_ROW:
  case TYPE_COLUMN:
    for (size_type i = 0; i < m_containerSize; ++i)
      if (m_container[i]->is_width_dynamic())
        return true;

    return false;
  }

  return false;
}

bool
Frame::is_height_dynamic() const {
  switch (m_type) {
  case TYPE_NONE:
    return false;

  case TYPE_WINDOW:
    return m_window->is_active() && m_window->is_height_dynamic();

  case TYPE_ROW:
  case TYPE_COLUMN:
    for (size_type i = 0; i < m_containerSize; ++i)
      if (m_container[i]->is_height_dynamic())
        return true;

    return false;
  }

  return false;
}

Frame::pair_type
Frame::preferred_size() const {
  switch (m_type) {
  case TYPE_NONE:
    return pair_type(0, 0);

  case TYPE_WINDOW:
    if (m_window->is_active())
      return pair_type(std::max<uint32_t>(m_window->min_width(), 1), std::max<uint32_t>(m_window->min_height(), 1));
    else
      return pair_type(0, 0);

  case TYPE_ROW:
  case TYPE_COLUMN:
    {
      pair_type accum(0, 0);

      for (size_type i = 0; i < m_containerSize; ++i) {
        pair_type p = m_container[i]->preferred_size();
 
        accum.first += p.first;
        accum.second += p.second;
      }

      return accum;
    }
  }

  return pair_type(0, 0);
}

void
Frame::set_container_size(size_type size) {
  if ((m_type != TYPE_ROW && m_type != TYPE_COLUMN) || size >= max_size)
    throw torrent::client_error("Frame::set_container_size(...) Bad state.");

  while (m_containerSize > size) {
    delete m_container[--m_containerSize];
    m_container[m_containerSize] = NULL;
  }

  while (m_containerSize < size) {
    m_container[m_containerSize++] = new Frame();
  }
}

void
Frame::initialize_window(Window* window) {
  if (m_type != TYPE_NONE)
    throw torrent::client_error("Frame::initialize_window(...) m_type != TYPE_NONE.");

  m_type = TYPE_WINDOW;
  m_window = window;
}

void
Frame::initialize_row(size_type size) {
  if (m_type != TYPE_NONE)
    throw torrent::client_error("Frame::initialize_container(...) Invalid state.");

  if (size > max_size)
    throw torrent::client_error("Frame::initialize_container(...) size >= max_size.");

  m_type = TYPE_ROW;
  m_containerSize = size;

  for (size_type i = 0; i < m_containerSize; ++i)
    m_container[i] = new Frame();
}

void
Frame::initialize_column(size_type size) {
  if (m_type != TYPE_NONE)
    throw torrent::client_error("Frame::initialize_container(...) Invalid state.");

  if (size > max_size)
    throw torrent::client_error("Frame::initialize_container(...) size >= max_size.");

  m_type = TYPE_COLUMN;
  m_containerSize = size;

  for (size_type i = 0; i < m_containerSize; ++i)
    m_container[i] = new Frame();
}

void
Frame::clear() {
  switch (m_type) {
  case TYPE_ROW:
  case TYPE_COLUMN:
    for (size_type i = 0; i < m_containerSize; ++i) {
      m_container[i]->clear();
      delete m_container[i];
    }
    break;

  default:
    break;
  }

  m_type = TYPE_NONE;
}

void
Frame::refresh() {
  switch (m_type) {
  case TYPE_NONE:
    break;

  case TYPE_WINDOW:
    if (m_window->is_active() && !m_window->is_offscreen())
      m_window->refresh();

    break;

  case TYPE_ROW:
  case TYPE_COLUMN:
    for (Frame **itr = m_container, **last = m_container + m_containerSize; itr != last; ++itr)
      (*itr)->refresh();

    break;
  }
}

void
Frame::balance(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  m_positionX = x;
  m_positionY = y;
  m_width = width;
  m_height = height;

  if (m_type == TYPE_NONE)
    return;

  if (m_type == TYPE_WINDOW) {
    // Ensure that we don't draw windows that are offscreen or have
    // zero extent.
    if (width == 0 || height == 0 || !m_window->is_active()) {
      m_window->set_offscreen(true);
      return;
    }

    m_window->set_offscreen(false);
    m_window->resize(x, y, width, height);
    m_window->mark_dirty();
    return;
  }

  // Find the size of the static frames. The dynamic frames are added
  // to a temporary list for the second pass. Each frame uses the
  // m_width and m_height as temporary storage for width and height in
  // this algorithm.
  size_type dynamicSize = 0;
  Frame* dynamicFrames[max_size];

  int remaining = m_type == TYPE_ROW ? height : width;
  
  for (Frame **itr = m_container, **last = m_container + m_containerSize; itr != last; ++itr) {
    pair_type p = (*itr)->preferred_size();
    (*itr)->m_width = p.first;
    (*itr)->m_height = p.second;

    if ((m_type == TYPE_ROW && (*itr)->is_height_dynamic()) || (m_type == TYPE_COLUMN && (*itr)->is_width_dynamic()))
      dynamicFrames[dynamicSize++] = *itr;
    else
      remaining -= m_type == TYPE_ROW ? p.second : p.first;
  }
  
  // Sort the dynamic frames by the min size in the direction we are
  // interested in. Then try to satisfy the largest first, and if we
  // have any remaining space we can use that to extend it and any
  // following frames.
  //
  // Else if we're short, only give each what they require.

  if (m_type == TYPE_ROW)
    std::sort(dynamicFrames, dynamicFrames + dynamicSize, rak::greater2(rak::mem_ptr(&Frame::m_height), rak::mem_ptr(&Frame::m_height)));
  else
    std::sort(dynamicFrames, dynamicFrames + dynamicSize, rak::greater2(rak::mem_ptr(&Frame::m_width), rak::mem_ptr(&Frame::m_width)));

  for (Frame **itr = dynamicFrames, **last = dynamicFrames + dynamicSize; itr != last; ++itr, --dynamicSize) {
    uint32_t s = std::max<uint32_t>((std::max(remaining, 0) + dynamicSize - 1) / dynamicSize,
                                    m_type == TYPE_ROW ? (*itr)->m_height : (*itr)->m_width);
    
    remaining -= s;
    
    if (m_type == TYPE_ROW)
      (*itr)->m_height = s;
    else
      (*itr)->m_width = s;
  }

  for (Frame **itr = m_container, **last = m_container + m_containerSize; itr != last; ++itr) {
    if (m_type == TYPE_ROW) {
      (*itr)->balance(x, y, m_width, std::min((*itr)->m_height, height));

      y += (*itr)->m_height;
      height -= (*itr)->m_height;

    } else {
      (*itr)->balance(x, y, std::min((*itr)->m_width, width), m_height);

      x += (*itr)->m_width;
      width -= (*itr)->m_width;
    }
  }
}

}
