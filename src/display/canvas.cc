#include "config.h"

#include "canvas.h"

namespace display {

void
Canvas::resize(int x, int y, int w, int h) {
  delwin(m_window);
  m_window = newwin(h, w, y, x);
}

void
Canvas::init() {
  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  curs_set(0);
}

void
Canvas::cleanup() {
  endwin();
}

}
