#include "config.h"

#include <stdexcept>

#include "window.h"

namespace display {

Window::Slot Window::m_slotAdjust;

Window::~Window() {
  delete m_canvas;
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
