#ifndef RTORRENT_DISPLAY_WINDOW_STATUSBAR_H
#define RTORRENT_DISPLAY_WINDOW_STATUSBAR_H

#include <cstdint>

#include "window.h"

namespace display {

class WindowStatusbar : public Window {
public:
  WindowStatusbar() :
    Window(new Canvas, 0, 0, 1, extent_full, extent_static),
    m_lastTick(0) {}

  virtual void   redraw();

private:
  uint64_t       m_lastTick;
};

}

#endif
