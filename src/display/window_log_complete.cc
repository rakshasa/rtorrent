#include "config.h"

#include <ctime>

#include "canvas.h"
#include "utils.h"
#include "window_log_complete.h"

namespace display {

WindowLogComplete::WindowLogComplete(torrent::log_buffer* l) :
  Window(new Canvas, 0, 30, 1, extent_full, extent_full),
  m_log(l) {
}

WindowLogComplete::~WindowLogComplete() = default;

WindowLogComplete::iterator
WindowLogComplete::find_older() {
  return m_log->find_older(torrent::this_thread::cached_seconds().count() - 60);
}

void
WindowLogComplete::redraw() {
  m_canvas->erase();

  if (m_canvas->width() < 16)
    return;

  int pos = m_canvas->height();

  for (iterator itr = m_log->end(), last = m_log->begin(); itr != last && pos > 0; ) {
    itr--;

    char buffer[16];

    // Use an arbitrary min width of 60 for allowing multiple
    // lines. This should ensure we don't mess up the display when the
    // screen is shrunk too much.
    unsigned int timeWidth = 3 + print_hhmmss_local(buffer, buffer + 16, static_cast<time_t>(itr->timestamp)) - buffer;

    unsigned int logWidth  = m_canvas->width() > 60 ? (m_canvas->width() - timeWidth) : (60 - timeWidth);
    unsigned int logHeight = (itr->message.size() + logWidth - 1) / logWidth;

    for (unsigned int j = logHeight; j > 0 && pos > 0; --j, --pos)
      if (j == 1)
        m_canvas->print(0, pos - 1, "(%s) %s", buffer, itr->message.substr(0, m_canvas->width() - timeWidth).c_str());
      else
        m_canvas->print(timeWidth, pos - 1, "%s", itr->message.substr(logWidth * (j - 1), m_canvas->width() - timeWidth).c_str());
  }
}

}
