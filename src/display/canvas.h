#ifndef RTORRENT_DISPLAY_CANVAS_H
#define RTORRENT_DISPLAY_CANVAS_H

#include <string>
#include <ncurses.h>

namespace display {

class Canvas {
public:
  Canvas(int x, int y, int width = 0, int height = 0) :
    m_window(newwin(height, width, y, x)) {}
  ~Canvas() { delwin(m_window); }

  void      refresh()                                          { wnoutrefresh(m_window); }
  void      redraw()                                           { redrawwin(m_window); }

  void      update_size(int w, int h)                          { wresize(m_window, h, w); }

  int       get_x()                                            { int x, y; getyx(m_window, y, x); return x; }
  int       get_y()                                            { int x, y; getyx(m_window, y, x); return y; }
  int       get_width()                                        { int x, y; getmaxyx(m_window, y, x); return x; }
  int       get_height()                                       { int x, y; getmaxyx(m_window, y, x); return y; }

  chtype    get_background()                                   { return getbkgd(m_window); }
  void      set_background(chtype c)                           { return wbkgdset(m_window, c); }

  void      erase()                                            { werase(m_window); }

  void      print(int x, int y, const char* str)               { mvwprintw(m_window, y, x, str); }

  template <typename A1>
  void      print(int x, int y, const char* str, A1 a1)        { mvwprintw(m_window, y, x, str, a1); }

  template <typename A1, typename A2>
  void      print(int x, int y, const char* str, A1 a1, A2 a2) { mvwprintw(m_window, y, x, str, a1, a2); }

  void      print_border(chtype ls, chtype rs,
			 chtype ts, chtype bs,
			 chtype tl, chtype tr,
			 chtype bl, chtype br)                 { wborder(m_window, ls, rs, ts, bs, tl, tr, bl, br); }

  // Initialize stdscr.
  static void init();
  static void cleanup();

  static void do_update()                                      { doupdate(); }

private:
  WINDOW*   m_window;
};

}

#endif
