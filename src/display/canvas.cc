#include "config.h"

#include "canvas.h"
#include <stdarg.h>

namespace display {

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
