#ifndef RTORRENT_DISPLAY_WINDOW_TITLE_H
#define RTORRENT_DISPLAY_WINDOW_TITLE_H

#include "window.h"

namespace display {

class WindowTitle : public Window {
public:
  WindowTitle();

  virtual void redraw();

private:
  std::string  m_title;
};

}

#endif
