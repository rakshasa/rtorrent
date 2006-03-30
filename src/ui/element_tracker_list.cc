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
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>

#include "display/window_tracker_list.h"
#include "input/manager.h"

#include "control.h"
#include "element_tracker_list.h"

namespace ui {

ElementTrackerList::ElementTrackerList(core::Download* d) :
  m_download(d),
  m_window(NULL),
  m_focus(0) {

  m_bindings[KEY_DOWN] = sigc::mem_fun(*this, &ElementTrackerList::receive_next);
  m_bindings[KEY_UP]   = sigc::mem_fun(*this, &ElementTrackerList::receive_prev);
  m_bindings[' ']      = sigc::mem_fun(*this, &ElementTrackerList::receive_cycle_group);
  m_bindings['*']      = sigc::mem_fun(*this, &ElementTrackerList::receive_disable);
}

void
ElementTrackerList::activate(Control* c, MItr mItr) {
  if (m_window != NULL)
    throw torrent::internal_error("ui::ElementTrackerList::activate(...) called on an object in the wrong state");

  c->input()->push_front(&m_bindings);

  *mItr = m_window = new WTrackerList(m_download, &m_focus);
}

void
ElementTrackerList::disable(Control* c) {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::disable(...) called on an object in the wrong state");

  c->input()->erase(&m_bindings);

  delete m_window;
  m_window = NULL;
}

void
ElementTrackerList::receive_disable() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_disable(...) called on a disabled object");

  torrent::Tracker t = m_download->get_download().tracker_list().get(m_focus);

  if (t.is_enabled())
    t.disable();
  else
    t.enable();

  m_window->mark_dirty();
}

void
ElementTrackerList::receive_next() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_next(...) called on a disabled object");

  if (++m_focus >= m_download->get_download().tracker_list().size())
    m_focus = 0;

  m_window->mark_dirty();
}

void
ElementTrackerList::receive_prev() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_prev(...) called on a disabled object");

  if (m_download->get_download().tracker_list().size() == 0)
    return;

  if (m_focus != 0)
    --m_focus;
  else 
    m_focus = m_download->get_download().tracker_list().size() - 1;

  m_window->mark_dirty();
}

void
ElementTrackerList::receive_cycle_group() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementTrackerList::receive_group_cycle(...) called on a disabled object");

  torrent::TrackerList tl = m_download->get_download().tracker_list();

  if (m_focus >= tl.size())
    throw torrent::internal_error("ui::ElementTrackerList::receive_group_cycle(...) called with an invalid focus");

  tl.cycle_group(tl.get(m_focus).group());

  m_window->mark_dirty();
}

}
