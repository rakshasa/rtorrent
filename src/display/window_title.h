#ifndef RTORRENT_DISPLAY_WINDOW_TITLE_H
#define RTORRENT_DISPLAY_WINDOW_TITLE_H

#include <string>
#include "window.h"

namespace display {

class WindowTitle : public Window {
public:
  WindowTitle() : Window(new Canvas, 0, 0, 1, extent_full, extent_static) {}

  const std::string&  title() const                       { return m_title; }
  void                set_title(const std::string& title) { m_title = title; mark_dirty(); }

  virtual void        redraw();

private:
  std::string         m_title;
};

}

#endif
