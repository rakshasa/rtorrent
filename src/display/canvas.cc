#include "config.h"

#include "canvas.h"

namespace display {

void
Canvas::resize(int x, int y, int w, int h) {
  wresize(m_window, h, w);
  mvwin(m_window, y, x);
}

void
Canvas::init() {
  initscr();
  raw();
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  curs_set(0);
}

void
Canvas::cleanup() {
  noraw();
  endwin();
}

}
