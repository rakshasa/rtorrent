#include "config.h"

#include "canvas.h"
#include "window_downloads.h"

namespace display {

WindowDownloads::WindowDownloads(engine::Downloads* d) :
  Window(new Canvas, true),
  m_downloads(d) {
}

void
WindowDownloads::redraw() {
  m_canvas->erase();
  m_canvas->print_border(' ', ' ', '-', '-', ' ', ' ', ' ', ' ');

  int pos = 1;

  for (engine::Downloads::iterator itr = m_downloads->begin(); itr != m_downloads->end(); ++itr, ++pos)
    m_canvas->print(1, pos, "Download: %s", itr->get_name().c_str());
}

}
