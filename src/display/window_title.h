#ifndef RTORRENT_DISPLAY_WINDOW_TITLE_H
#define RTORRENT_DISPLAY_WINDOW_TITLE_H

#include <string>
#include "window.h"

namespace display {

class WindowTitle : public Window {
public:
  WindowTitle(const std::string& s);

  virtual void redraw();

private:
  std::string  m_title;
};

}

#endif
