#ifndef RTORRENT_DISPLAY_CANVAS_H
#define RTORRENT_DISPLAY_CANVAS_H

#include <string>
#include <ncurses.h>

namespace display {

class Canvas {
public:
  Canvas(int x = 0, int y = 0, int width = 0, int height = 0) :
    m_window(newwin(height, width, y, x)) {}
  ~Canvas() { delwin(m_window); }

  void        refresh()                                        { wnoutrefresh(m_window); }
  static void refresh_std()                                    { wnoutrefresh(stdscr); }
  void        redraw()                                         { redrawwin(m_window); }

  void        resize(int w, int h)                             { wresize(m_window, h, w); }
  void        resize(int x, int y, int w, int h);

  int         get_x()                                          { int x, y; getyx(m_window, y, x); return x; }
  int         get_y()                                          { int x, y; getyx(m_window, y, x); return y; }
  int         get_width()                                      { int x, y; getmaxyx(m_window, y, x); return x; }
  int         get_height()                                     { int x, y; getmaxyx(m_window, y, x); return y; }

  chtype      get_background()                                 { return getbkgd(m_window); }
  void        set_background(chtype c)                         { return wbkgdset(m_window, c); }

  void        erase()                                          { werase(m_window); }

  void        print_border(chtype ls, chtype rs,
			   chtype ts, chtype bs,
			   chtype tl, chtype tr,
			   chtype bl, chtype br)               { wborder(m_window, ls, rs, ts, bs, tl, tr, bl, br); }

  void print(int x, int y, const char* str)                    { mvwprintw(m_window, y, x, str); }

  template <typename A1>
  void print(int x, int y, const char* str, A1 a1)             { mvwprintw(m_window, y, x, str, a1); }

  template <typename A1, typename A2>
  void print(int x, int y, const char* str, A1 a1, A2 a2)      { mvwprintw(m_window, y, x, str, a1, a2); }

  template <typename A1, typename A2, typename A3>
  void print(int x, int y, const char* str,
		  A1 a1, A2 a2, A3 a3)                         { mvwprintw(m_window, y, x, str, a1, a2, a3); }

  template <typename A1, typename A2, typename A3, typename A4>
  void print(int x, int y, const char* str,
		  A1 a1, A2 a2, A3 a3, A4 a4)                  { mvwprintw(m_window, y, x, str, a1, a2, a3, a4); }

  template <typename A1, typename A2, typename A3, typename A4, typename A5>
  void print(int x, int y, const char* str,
		  A1 a1, A2 a2, A3 a3, A4 a4, A5 a5)           { mvwprintw(m_window, y, x, str, a1, a2, a3, a4, a5); }

  template <typename A1, typename A2, typename A3, typename A4, typename A5, typename A6>
  void print(int x, int y, const char* str,
		  A1 a1, A2 a2, A3 a3, A4 a4, A5 a5, A6 a6)    { mvwprintw(m_window, y, x, str, a1, a2, a3, a4, a5, a6); }

  void set_attr(int x, int y, int n, int attr, int color)      { mvwchgat(m_window, y, x, n, attr, color, NULL); }

  // Initialize stdscr.
  static void init();
  static void cleanup();

  static int  get_screen_width()                               { int x, y; getmaxyx(stdscr, y, x); return x; }
  static int  get_screen_height()                              { int x, y; getmaxyx(stdscr, y, x); return y; }

  static void do_update()                                      { doupdate(); }

private:
  Canvas(const Canvas&);
  void operator = (const Canvas&);

  WINDOW*   m_window;
};

}

#endif
