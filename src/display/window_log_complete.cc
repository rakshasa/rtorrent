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

#include <ctime>

#include "canvas.h"
#include "utils.h"
#include "window_log_complete.h"

namespace display {

WindowLogComplete::WindowLogComplete(core::Log* l) :
  Window(new Canvas, 0, 30, 1, extent_full, extent_full),
  m_log(l) {

  // We're trying out scheduled tasks instead.
  m_connUpdate = l->signal_update().connect(sigc::mem_fun(*this, &WindowLogComplete::receive_update));
}

WindowLogComplete::~WindowLogComplete() {
  m_connUpdate.disconnect();
}

WindowLogComplete::iterator
WindowLogComplete::find_older() {
  return m_log->find_older(cachedTime - rak::timer::from_seconds(60));
}

void
WindowLogComplete::redraw() {
  m_canvas->erase();

  if (m_canvas->width() < 16)
    return;

  int pos = m_canvas->height();

  for (core::Log::iterator itr = m_log->begin(), last = m_log->end(); itr != last && pos > 0; ++itr) {
    char buffer[16];

    // Use an arbitrary min width of 60 for allowing multiple
    // lines. This should ensure we don't mess up the display when the
    // screen is shrunk too much.
    unsigned int timeWidth = 3 + print_hhmmss_local(buffer, buffer + 16, static_cast<time_t>(itr->first.seconds())) - buffer;

    unsigned int logWidth  = m_canvas->width() > 60 ? (m_canvas->width() - timeWidth) : (60 - timeWidth);
    unsigned int logHeight = (itr->second.size() + logWidth - 1) / logWidth;

    for (unsigned int j = logHeight; j > 0 && pos > 0; --j, --pos)
      if (j == 1)
        m_canvas->print(0, pos - 1, "(%s) %s", buffer, itr->second.substr(0, m_canvas->width() - timeWidth).c_str());
      else
        m_canvas->print(timeWidth, pos - 1, "%s", itr->second.substr(logWidth * (j - 1), m_canvas->width() - timeWidth).c_str());
  }
}

void
WindowLogComplete::receive_update() {
  mark_dirty();
}

}
