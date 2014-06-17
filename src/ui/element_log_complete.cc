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
#include <torrent/torrent.h>
#include <torrent/utils/thread_base.h>

#include "display/frame.h"
#include "display/manager.h"
#include "display/window_log_complete.h"
#include "input/manager.h"

#include "control.h"
#include "element_log_complete.h"

namespace ui {

ElementLogComplete::ElementLogComplete(torrent::log_buffer* l) :
  m_window(NULL),
  m_log(l) {

  unsigned int signal_index = torrent::main_thread()->signal_bitfield()->add_signal(std::bind(&ElementLogComplete::received_update, this));

  m_log->lock_and_set_update_slot(std::bind(&torrent::thread_base::send_event_signal, torrent::main_thread(), signal_index, false));
}

void
ElementLogComplete::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::ElementLogComplete::activate(...) is_active().");

  control->input()->push_back(&m_bindings);

  m_window = new WLogComplete(m_log);
  m_window->set_active(true);

  m_frame = frame;
  m_frame->initialize_window(m_window);
}

void
ElementLogComplete::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::ElementLogComplete::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  delete m_window;
  m_window = NULL;
}

display::Window*
ElementLogComplete::window() {
  return m_window;
}

void
ElementLogComplete::received_update() {
  if (m_window != NULL)
    m_window->mark_dirty();
}

}
