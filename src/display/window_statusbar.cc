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
  m_counter++;

  if (utils::Timer::cache() - m_lastDraw < 10000000)
    return;

  m_lastDraw = utils::Timer::cache();

  m_canvas->erase();
  m_canvas->print(0, 0, "Throttle: %i Listen: %s:%i Handshakes: %i Select: %u",
		  (int)torrent::get(torrent::THROTTLE_ROOT_CONST_RATE) / 1024,
		  "",
		  (int)torrent::get(torrent::LISTEN_PORT),
		  (int)torrent::get(torrent::HANDSHAKES_TOTAL),
		  m_counter);
}

}
