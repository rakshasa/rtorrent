#ifndef RTORRENT_WINDOW_BASE_H
#define RTORRENT_WINDOW_BASE_H

#include <sigc++/slot.h>

#include "canvas.h"
#include "utils/timer.h"

namespace display {

class Canvas;

class Window {
public:
  typedef sigc::slot0<void> Slot;

  Window(Canvas* c = NULL, bool d = false, int h = 1) :
    m_canvas(c), m_active(true), m_dynamic(d), m_minHeight(h) {}

  virtual ~Window();

  bool         is_active()                          { return m_active; }
  bool         is_dynamic()                         { return m_dynamic; }
  bool         is_dirty()                           { return m_nextDraw <= utils::Timer::cache(); }

  utils::Timer get_next_draw()                      { return m_nextDraw; }

  int          get_min_height()                     { return m_minHeight; }

  bool         get_active()                         { return m_active; }
  void         set_active(bool a)                   { m_active = a; }

  void         refresh()                            { m_canvas->refresh(); }
  void         resize(int x, int y, int w, int h);

  void         mark_dirty()                         { m_nextDraw = utils::Timer::min(); }

  virtual void redraw() = 0;

  static void  slot_adjust(Slot s)                  { m_slotAdjust = s; }

protected:
  Window(const Window&);
  void operator = (const Window&);

  static Slot  m_slotAdjust;

  Canvas*      m_canvas;

  bool         m_active;
  bool         m_dynamic;
  int          m_minHeight;

  utils::Timer m_nextDraw;
};

}

#endif

