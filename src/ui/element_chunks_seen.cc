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

#include "config.h"

#include <torrent/exceptions.h>

#include "display/frame.h"
#include "display/window_download_chunks_seen.h"
#include "input/manager.h"

#include "control.h"
#include "element_chunks_seen.h"

namespace ui {

ElementChunksSeen::ElementChunksSeen(core::Download* d) :
  m_download(d),
  m_window(NULL),
  m_focus(0) {

  m_bindings[KEY_LEFT] = m_bindings['B' - '@'] = std::tr1::bind(&slot_type::operator(), &m_slot_exit);

  m_bindings[KEY_DOWN]  = m_bindings['N' - '@'] = std::tr1::bind(&ElementChunksSeen::receive_next, this);
  m_bindings[KEY_UP]    = m_bindings['P' - '@'] = std::tr1::bind(&ElementChunksSeen::receive_prev, this);
  m_bindings[KEY_NPAGE] = std::tr1::bind(&ElementChunksSeen::receive_pagenext, this);
  m_bindings[KEY_PPAGE] = std::tr1::bind(&ElementChunksSeen::receive_pageprev, this);
}

void
ElementChunksSeen::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::ElementChunksSeen::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_window = new WChunksSeen(m_download, &m_focus);
  m_window->set_active(true);

  m_frame = frame;
  m_frame->initialize_window(m_window);
}

void
ElementChunksSeen::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::ElementChunksSeen::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

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
    throw torrent::internal_error("ui::ElementChunksSeen::receive_next(...) called on a disabled object");

  if (++m_focus > m_window->max_focus())
    m_focus = 0;

  m_window->mark_dirty();
}

void
ElementChunksSeen::receive_prev() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementChunksSeen::receive_prev(...) called on a disabled object");

  if (m_focus > 0)
    --m_focus;
  else
    m_focus = m_window->max_focus();

  m_window->mark_dirty();
}

void
ElementChunksSeen::receive_pagenext() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementChunksSeen::receive_pagenext(...) called on a disabled object");

  unsigned int visible = m_window->height() - 1;
  unsigned int maxFocus = m_window->max_focus();

  if (maxFocus == 0 || m_focus == maxFocus)
    m_focus = 0;
  else if (m_focus + visible / 2 < maxFocus)
    m_focus += visible / 2;
  else 
    m_focus = maxFocus;

  m_window->mark_dirty();
}

void
ElementChunksSeen::receive_pageprev() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementChunksSeen::receive_pageprev(...) called on a disabled object");

  unsigned int visible = m_window->height() - 1;
  unsigned int maxFocus = m_window->max_focus();

  if (m_focus > visible / 2)
    m_focus -= visible / 2;
  else if (maxFocus > 0 && m_focus == 0)
    m_focus = maxFocus;
  else
    m_focus = 0;

  m_window->mark_dirty();
}

}
