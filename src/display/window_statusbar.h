#ifndef RTORRENT_DISPLAY_WINDOW_STATUSBAR_H
#define RTORRENT_DISPLAY_WINDOW_STATUSBAR_H

#include "window.h"

namespace core {
  class Manager;
}

namespace display {

class WindowStatusbar : public Window {
public:
  WindowStatusbar(core::Manager* c);

  virtual void redraw();

private:
  int            m_counter;
  core::Manager* m_core;
};

}

#endif
