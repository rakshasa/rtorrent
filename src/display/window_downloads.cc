#include "config.h"

#include "window_downloads.h"
#include "canvas.h"

namespace display {

WindowDownloads::WindowDownloads(Downloads* d) :
  Window(new Canvas, true),
  m_downloads(d) {
}

void
WindowDownloads::redraw() {
  m_canvas->erase();
  m_canvas->print_border(' ', ' ', '-', '-', ' ', ' ', ' ', ' ');

  int pos = 1;

  for (Downloads::iterator itr = m_downloads->begin(); itr != m_downloads->end(); ++itr, ++pos)
    m_canvas->print(1, pos, "Download: %s", itr->get_name().c_str());
}

}
