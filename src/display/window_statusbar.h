#ifndef RTORRENT_DISPLAY_WINDOW_STATUSBAR_H
#define RTORRENT_DISPLAY_WINDOW_STATUSBAR_H

#include "window.h"

namespace display {

class WindowStatusbar : public Window {
public:
  WindowStatusbar();

  virtual void redraw();

private:
  int m_counter;
};

}

#endif
