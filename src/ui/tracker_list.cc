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

#include <stdexcept>

#include "display/window_tracker_list.h"

#include "control.h"
#include "tracker_list.h"

namespace ui {

TrackerList::TrackerList(Control* c, core::Download* d) :
  m_download(d),
  m_window(NULL),
  m_control(c),
  m_focus(0) {

  m_bindings[KEY_DOWN] = sigc::mem_fun(*this, &TrackerList::receive_next);
  m_bindings[KEY_UP] = sigc::mem_fun(*this, &TrackerList::receive_prev);
}

void
TrackerList::activate(MItr mItr) {
  if (m_window != NULL)
    throw std::logic_error("ui::TrackerList::activate(...) called on an object in the wrong state");

  m_control->get_input().push_front(&m_bindings);

  *mItr = m_window = new WTrackerList(m_download, &m_focus);
}

void
TrackerList::disable() {
  if (m_window == NULL)
    throw std::logic_error("ui::TrackerList::disable(...) called on an object in the wrong state");

  m_control->get_input().erase(&m_bindings);

  delete m_window;
  m_window = NULL;
}

void
TrackerList::receive_next() {
  if (m_window == NULL)
    throw std::logic_error("ui::TrackerList::receive_next(...) called on a disabled object");

  if (++m_focus >= m_download->get_download().get_tracker_size())
    m_focus = 0;

  m_window->mark_dirty();
}

void
TrackerList::receive_prev() {
  if (m_window == NULL)
    throw std::logic_error("ui::TrackerList::receive_prev(...) called on a disabled object");

  if (m_download->get_download().get_tracker_size() == 0)
    return;

  if (m_focus != 0)
    --m_focus;
  else 
    m_focus = m_download->get_download().get_tracker_size() - 1;

  m_window->mark_dirty();
}

}
