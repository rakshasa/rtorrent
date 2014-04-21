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
#include "input/manager.h"

#include "control.h"
#include "element_string_list.h"

namespace ui {

ElementStringList::ElementStringList() :
  m_window(NULL) {
}

void
ElementStringList::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::ElementStringList::activate(...) is_active().");

  lt_log_print(torrent::LOG_UI_EVENTS, "element_string_list: activate");

  control->input()->push_back(&m_bindings);

  m_window = new WStringList();
  m_window->set_active(true);

  m_frame = frame;
  m_frame->initialize_window(m_window);
}

void
ElementStringList::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::ElementStringList::disable(...) !is_active().");

  lt_log_print(torrent::LOG_UI_EVENTS, "element_string_list: deactivate");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  delete m_window;
  m_window = NULL;
}

void
ElementStringList::next_screen() {
  if (m_window == NULL)
    return;

  if (m_window->get_draw_end() != m_list.end())
    m_window->set_range(m_window->get_draw_end(), m_list.end());
  else
    m_window->set_range(m_list.begin(), m_list.end());

  m_window->mark_dirty();
}

}
