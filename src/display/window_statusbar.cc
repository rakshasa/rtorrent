#include "config.h"

#include <torrent/torrent.h>

#include "canvas.h"
#include "window_statusbar.h"

namespace display {

WindowStatusbar::WindowStatusbar() :
  Window(new Canvas, false, 1),
  m_counter(0) {
}

void
WindowStatusbar::redraw() {
  m_canvas->erase();
  m_canvas->print(0, 0, "Loop: %i", ++m_counter);
}

}
