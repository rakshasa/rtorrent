#ifndef RTORRENT_WINDOW_BASE_H
#define RTORRENT_WINDOW_BASE_H

#include "timer.h"

namespace display {

class Canvas;

class Window {
public:
  Window(Canvas* c = NULL, bool d = false, int h = 1) :
    m_canvas(c), m_dynamic(d), m_minHeight(h) {}

  virtual ~Window();

  bool         is_dynamic()     { return m_dynamic; }
  bool         is_dirty()       { return m_lastDraw == 0; }

  int          get_min_height() { return m_minHeight; }

  void         refresh();
  void         resize(int x, int y, int w, int h);

  void         mark_dirty()     { m_lastDraw = 0; }

  virtual void redraw() = 0;

protected:
  Window(const Window&);
  void operator = (const Window&);

  Canvas*      m_canvas;

  bool         m_dynamic;
  int          m_minHeight;

  Timer        m_lastDraw;
};

}

#endif

