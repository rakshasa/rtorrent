#include "config.h"

#include <torrent/torrent.h>

#include "canvas.h"
#include "window_title.h"

namespace display {

WindowTitle::WindowTitle() :
  Window(new Canvas, false, 1) {

  m_title += "rtorrent " VERSION " - ";
  m_title += torrent::get(torrent::LIBRARY_NAME);
}

void
WindowTitle::redraw() {
  m_canvas->erase();

  m_canvas->print(std::max(0, (m_canvas->get_width() - (int)m_title.size()) / 2 - 4), 0,
		  "*** %s ***", m_title.c_str());
}

}
