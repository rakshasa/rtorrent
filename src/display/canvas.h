#ifndef RTORRENT_DISPLAY_CANVAS_H
#define RTORRENT_DISPLAY_CANVAS_H

#include <cstdarg>
#include <string>
#include <unordered_map>
#include <vector>

#include "attributes.h"
#include "color_map.h"

namespace display {

class Canvas {
public:
  typedef std::vector<Attributes>      attributes_list;
  typedef std::unordered_map<int, int> attributes_map;

  Canvas(int x = 0, int y = 0, int width = 0, int height = 0);
  ~Canvas();

  void         refresh();
  void         redraw();
  void         resize(int w, int h);
  void         resize(int x, int y, int w, int h);

  static void  refresh_std();
  static void  redraw_std();
  static void  resize_term(int x, int y);
  static void  resize_term(std::pair<int, int> dim);

  unsigned int get_x();
  unsigned int get_y();
  unsigned int width();
  unsigned int height();

  void         move(unsigned int x, unsigned int y);
  void         erase();
  static void  erase_std();

  // The format string is non-const, but that will not be a problem
  // since the string shall always be a C string choosen at
  // compiletime. Might cause extra copying of the string?

  void print(const char* str, ...);
  void print(unsigned int x, unsigned int y, const char* str, ...);
  void print_attributes(unsigned int x, unsigned int y, const char* first, const char* last, const attributes_list* attributes);
  void print_char(const chtype ch);
  void print_char(unsigned int x, unsigned int y, const chtype ch);

  void set_attr(unsigned int x, unsigned int y, unsigned int n, int attr, int color);
  void set_attr(unsigned int x, unsigned int y, unsigned int n, ColorKind k);
  void set_default_attributes(int attr);

  // Initialize stdscr.
  static void initialize();
  static void build_colors();
  static void cleanup();

  static int get_screen_width();
  static int get_screen_height();

  static std::pair<int, int> term_size();

  static void do_update();

  static bool                  daemon() { return m_isDaemon; }
  static const attributes_map& attr_map() { return m_attr_map; }

private:
  Canvas(const Canvas&);
  void        operator=(const Canvas&);

  static bool m_isInitialized;
  static bool m_isDaemon;

  // Maps ncurses color IDs to a ncurses attribute int
  static std::unordered_map<int, int> m_attr_map;

  WINDOW*                             m_window;
};

inline Canvas::~Canvas() {
  if (!m_isDaemon) {
    delwin(m_window);
  }
}

inline void
Canvas::refresh() {
  if (!m_isDaemon) {
    wnoutrefresh(m_window);
  }
}

inline void
Canvas::refresh_std() {
  if (!m_isDaemon) {
    wnoutrefresh(stdscr);
  }
}

inline void
Canvas::redraw() {
  if (!m_isDaemon) {
    redrawwin(m_window);
  }
}

inline void
Canvas::redraw_std() {
  if (!m_isDaemon) {
    redrawwin(stdscr);
  }
}

inline void
Canvas::resize(int w, int h) {
  if (!m_isDaemon) {
    wresize(m_window, h, w);
  }
}

inline void
Canvas::resize_term(int x, int y) {
  if (!m_isDaemon) {
    resizeterm(y, x);
  }
}

inline void
Canvas::resize_term(std::pair<int, int> dim) {
  if (!m_isDaemon) {
    resizeterm(dim.second, dim.first);
  }
}

inline unsigned int
Canvas::get_x() {
  int x;
  [[maybe_unused]] int y;
  if (!m_isDaemon) {
    getyx(m_window, y, x);
  } else {
    x = 1;
  }
  return x;
}

inline unsigned int
Canvas::get_y() {
  int x, y;
  if (!m_isDaemon) {
    getyx(m_window, y, x);
  } else {
    y = 1;
  }
  return y;
}

inline unsigned int
Canvas::width() {
  int x;
  [[maybe_unused]] int y;
  if (!m_isDaemon) {
    getmaxyx(m_window, y, x);
  } else {
    x = 80;
  }
  return x;
}

inline unsigned int
Canvas::height() {
  int x, y;
  if (!m_isDaemon) {
    getmaxyx(m_window, y, x);
  } else {
    y = 24;
  }
  return y;
}

inline void
Canvas::move(unsigned int x, unsigned int y) {
  if (!m_isDaemon) {
    wmove(m_window, y, x);
  }
}

inline void
Canvas::erase() {
  if (!m_isDaemon) {
    werase(m_window);
  }
}

inline void
Canvas::erase_std() {
  if (!m_isDaemon) {
    werase(stdscr);
  }
}

inline void
Canvas::print(const char* str, ...) {
  va_list arglist;

  if (!m_isDaemon) {
    va_start(arglist, str);
    vw_printw(m_window, const_cast<char*>(str), arglist);
    va_end(arglist);
  }
}

inline void
Canvas::print(unsigned int x, unsigned int y, const char* str, ...) {
  va_list arglist;

  if (!m_isDaemon) {
    va_start(arglist, str);
    wmove(m_window, y, x);
    vw_printw(m_window, const_cast<char*>(str), arglist);
    va_end(arglist);
  }
}

inline void
Canvas::print_char(const chtype ch) {
  if (!m_isDaemon) {
    waddch(m_window, ch);
  }
}

inline void
Canvas::print_char(unsigned int x, unsigned int y, const chtype ch) {
  if (!m_isDaemon) {
    mvwaddch(m_window, y, x, ch);
  }
}

inline void
Canvas::set_attr(unsigned int x, unsigned int y, unsigned int n, int attr, int color) {
  if (!m_isDaemon) {
    mvwchgat(m_window, y, x, n, attr, color, NULL);
  }
}

inline void
Canvas::set_attr(unsigned int x, unsigned int y, unsigned int n, ColorKind k) {
  if (!m_isDaemon) {
    mvwchgat(m_window, y, x, n, m_attr_map[k], k, NULL);
  }
}

inline void
Canvas::set_default_attributes(int attr) {
  if (!m_isDaemon) {
    (void)wattrset(m_window, attr);
  }
}

inline int
Canvas::get_screen_width() {
  int x;
  [[maybe_unused]] int y;
  if (!m_isDaemon) {
    getmaxyx(stdscr, y, x);
  } else {
    x = 80;
  }
  return x;
}

inline int
Canvas::get_screen_height() {
  int x, y;
  if (!m_isDaemon) {
    getmaxyx(stdscr, y, x);
  } else {
    y = 24;
  }
  return y;
}

inline void
Canvas::do_update() {
  if (!m_isDaemon) {
    doupdate();
  }
}

} // namespace display

#endif
