#include "config.h"

#include <stdexcept>
#include <algo/call.h>
#include <algo/entry.h>
#include <algo/lambda.h>

#include "canvas.h"
#include "manager.h"
#include "window_base.h"

namespace display {

using namespace algo;

void
Manager::adjust_layout() {
  adjust_row(begin(), end(), 0, 0, display::Canvas::get_screen_width(), display::Canvas::get_screen_height());
}

void
Manager::adjust_row(iterator bItr, iterator eItr, int x, int y, int w, int h) {
  int t;
  int dist = std::distance(bItr, eItr);

  switch (dist) {
  case 1:
    bItr->get_window()->resize(x, y, w, h);
    break;

  case 2:
    t = w / 2 + w % 2;

    bItr->get_window()->resize(x, y, t, h);
    ++bItr;
    bItr->get_window()->resize(x + t, y, w - t, h);
    break;

  default:
    throw std::logic_error("Manager::adjust_row(...) got a range with invalid number of elements");
  }
}    

void
Manager::do_update() {
  std::for_each(begin(), end(), mem_fun(*mem_fun(_0(), &WindowBase::get_canvas), &Canvas::refresh));

  Canvas::do_update();
}

}
