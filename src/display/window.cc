#include "config.h"

#include "window_base.h"
#include "canvas.h"

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
