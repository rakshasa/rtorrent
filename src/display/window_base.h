#ifndef RTORRENT_WINDOW_BASE_H
#define RTORRENT_WINDOW_BASE_H

namespace display {

class Canvas;

class WindowBase {
public:
  WindowBase() : m_canvas(NULL) {}
  virtual ~WindowBase() {}

  Canvas*             get_canvas()          { return m_canvas; }
  void                set_canvas(Canvas* c) { m_canvas = c; }
  
  void                refresh();
  void                resize(int x, int y, int w, int h);

  virtual void        do_update() = 0;

protected:
  Canvas*             m_canvas;
};

}

#endif

