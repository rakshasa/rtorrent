#ifndef RTORRENT_WINDOW_BASE_H
#define RTORRENT_WINDOW_BASE_H

namespace display {

class Canvas;

class Window {
public:
  Window(Canvas* c = NULL) : m_canvas(c) {}
  virtual ~Window() {}

  Canvas*      get_canvas()          { return m_canvas; }
  
  void         refresh();
  void         resize(int x, int y, int w, int h);

  virtual void redraw() = 0;

protected:
  Window(const Window&);
  void operator = (const Window&);

  Canvas*             m_canvas;
};

}

#endif

