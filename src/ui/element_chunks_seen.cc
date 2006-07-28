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

#include <torrent/exceptions.h>

#include "display/window_download_chunks_seen.h"
#include "input/manager.h"

#include "control.h"
#include "element_chunks_seen.h"

namespace ui {

ElementChunksSeen::ElementChunksSeen(core::Download* d) :
  m_download(d),
  m_window(NULL),
  m_focus(0) {

  m_bindings[KEY_DOWN]  = sigc::mem_fun(*this, &ElementChunksSeen::receive_next);
  m_bindings[KEY_UP]    = sigc::mem_fun(*this, &ElementChunksSeen::receive_prev);
  m_bindings[KEY_NPAGE] = sigc::mem_fun(*this, &ElementChunksSeen::receive_pagenext);
  m_bindings[KEY_PPAGE] = sigc::mem_fun(*this, &ElementChunksSeen::receive_pageprev);
//   m_bindings[' ']      = sigc::mem_fun(*this, &ElementChunksSeen::receive_cycle_group);
//   m_bindings['*']      = sigc::mem_fun(*this, &ElementChunksSeen::receive_disable);
}

void
ElementChunksSeen::activate(display::Frame* frame) {
  if (is_active())
    throw torrent::client_error("ui::ElementChunksSeen::activate(...) is_active().");

  control->input()->push_front(&m_bindings);

  m_window = new WChunksSeen(m_download, &m_focus);
  m_frame = frame;
}

void
ElementChunksSeen::disable() {
  if (!is_active())
    throw torrent::client_error("ui::ElementChunksSeen::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  delete m_window;
  m_window = NULL;
}

display::Window*
ElementChunksSeen::window() {
  return m_window;
}

// void
// ElementChunksSeen::receive_disable() {
//   if (m_window == NULL)
//     throw std::logic_error("ui::ElementChunksSeen::receive_disable(...) called on a disabled object");

//   if (m_download->download()->tracker(m_focus).is_enabled())
//     m_download->download()->tracker(m_focus).disable();
//   else
//     m_download->download()->tracker(m_focus).enable();

//   m_window->mark_dirty();
// }

void
ElementChunksSeen::receive_next() {
  if (m_window == NULL)
    throw torrent::client_error("ui::ElementChunksSeen::receive_next(...) called on a disabled object");

  if (++m_focus > m_window->max_focus())
    m_focus = 0;

  m_window->mark_dirty();
}

void
ElementChunksSeen::receive_prev() {
  if (m_window == NULL)
    throw torrent::client_error("ui::ElementChunksSeen::receive_prev(...) called on a disabled object");

  if (m_focus > 0)
    --m_focus;
  else
    m_focus = m_window->max_focus();

  m_window->mark_dirty();
}

void
ElementChunksSeen::receive_pagenext() {
  if (m_window == NULL)
    throw torrent::client_error("ui::ElementChunksSeen::receive_pagenext(...) called on a disabled object");

  unsigned int visible = m_window->height() - 1;
  unsigned int scrollable = std::max<int>(m_window->rows() - visible, 0);

  if (scrollable == 0 || m_focus == scrollable)
    m_focus = 0;
  else if (m_focus + visible / 2 < scrollable)
    m_focus += visible / 2;
  else 
    m_focus = scrollable;

  m_window->mark_dirty();
}

void
ElementChunksSeen::receive_pageprev() {
  if (m_window == NULL)
    throw torrent::client_error("ui::ElementChunksSeen::receive_pageprev(...) called on a disabled object");

  unsigned int visible = m_window->height() - 1;
  unsigned int scrollable = std::max<int>(m_window->rows() - visible, 0);

  if (m_focus > visible / 2)
    m_focus -= visible / 2;
  else if (scrollable > 0 && m_focus == 0)
    m_focus = scrollable;
  else
    m_focus = 0;

  m_window->mark_dirty();
}

}
