#include "config.h"

#include "canvas.h"
#include "window_title.h"

namespace display {

void
WindowTitle::redraw() {
  if (m_canvas->daemon())
    return;

  schedule_update();
  m_canvas->erase();

  m_canvas->print(std::max(0, ((int)m_canvas->width() - (int)m_title.size()) / 2 - 4), 0, "*** %s ***", m_title.c_str());
}

} // namespace display
