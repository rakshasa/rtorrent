#ifndef RTORRENT_DISPLAY_MANAGER_ELEMENT_H
#define RTORRENT_DISPLAY_MANAGER_ELEMENT_H

namespace display {

class WindowBase;

class ManagerElement {
public:
  ManagerElement(WindowBase* w, int flags) : m_window(w), m_flags(flags) {}

  WindowBase*         get_window()              { return m_window; }
  void                set_window(WindowBase* w) { m_window = w; }

  int                 get_flags()               { return m_flags; }
  void                set_flags(int flags)      { m_flags = flags; }

private:
  WindowBase*         m_window;
  int                 m_flags;
};

}

#endif
