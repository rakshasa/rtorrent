#include "config.h"

#include "display/color_map.h"
#include <torrent/rate.h>
#include <torrent/torrent.h>

#include "control.h"
#include "canvas.h"
#include "utils.h"
#include "window_statusbar.h"

namespace display {

void
WindowStatusbar::redraw() {
  schedule_update();

  m_canvas->erase();

  std::vector<char> buffer(m_canvas->width() + 1);
  char* position;
  char* last = buffer.data() + m_canvas->width();

  position = print_status_info(buffer.data(), last);
  m_canvas->print(0, 0, "%s", buffer.data());

  last = last - (position - buffer.data());

  if (last > buffer.data()) {
    position = print_status_extra(buffer.data(), last);
    m_canvas->print(m_canvas->width() - (position - buffer.data()), 0, "%s", buffer.data());
  }

  m_canvas->set_attr(0, 0, -1, RCOLOR_FOOTER);

  m_lastTick = control->tick();
}

}
