#include "config.h"

#include <stdexcept>

#include "canvas.h"
#include "window.h"

namespace display {

void
Window::refresh() {
  m_canvas->refresh();
}

void
Window::resize(int x, int y, int w, int h) {
  if (x < 0 || y < 0)
    throw std::logic_error("Window::resize(...) bad x or y position");

  if (w <= 0 || h <= 0)
    throw std::logic_error("Window::resize(...) bad size");

  m_canvas->resize(x, y, w, h);
}

}
