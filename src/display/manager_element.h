#ifndef RTORRENT_DISPLAY_MANAGER_ELEMENT_H
#define RTORRENT_DISPLAY_MANAGER_ELEMENT_H

#include "window.h"

namespace display {

class ManagerElement {
public:
  ManagerElement(Window* w, int flags) : m_window(w), m_flags(flags) {}

  void      refresh()             { m_window->refresh(); }
  void      redraw()              { m_window->redraw(); }

  Window*   get_window()          { return m_window; }
  void      set_window(Window* w) { m_window = w; }

  int       get_flags()           { return m_flags; }
  void      set_flags(int flags)  { m_flags = flags; }

private:
  Window*   m_window;
  int       m_flags;
};

}

#endif
