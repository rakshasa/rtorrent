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
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());

  m_canvas->erase();

  // TODO: Make a buffer with size = get_width?
  char buffer[m_canvas->width() + 1];
  char* position;
  char* last = buffer + m_canvas->width();

  position = print_status_info(buffer, last);
  m_canvas->print(0, 0, "%s", buffer);

  last = last - (position - buffer);

  if (last > buffer) {
    position = print_status_extra(buffer, last);
    m_canvas->print(m_canvas->width() - (position - buffer), 0, "%s", buffer);
  }
  m_canvas->set_attr(0, 0, -1, RCOLOR_FOOTER);

  m_lastTick = control->tick();
}

}
