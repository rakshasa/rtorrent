#include "config.h"

#include "canvas.h"
#include "window.h"

namespace display {

void
Window::refresh() {
  m_canvas->refresh();
}

void
Window::resize(int x, int y, int w, int h) {
  m_canvas->resize(x, y, w, h);
}

}
