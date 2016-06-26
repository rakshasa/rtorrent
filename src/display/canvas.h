// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_DISPLAY_CANVAS_H
#define RTORRENT_DISPLAY_CANVAS_H

#include <string>
#include <vector>

#include "attributes.h"

namespace display {

class Canvas {
public:
  typedef std::vector<Attributes> attributes_list;

  Canvas(int x = 0, int y = 0, int width = 0, int height = 0);
  ~Canvas() { if (!m_isDaemon) { delwin(m_window); } }

  void                refresh()                                               { if (!m_isDaemon) { wnoutrefresh(m_window); } }
  static void         refresh_std()                                           { if (!m_isDaemon) { wnoutrefresh(stdscr); } }
  void                redraw()                                                { if (!m_isDaemon) { redrawwin(m_window); } }
  static void         redraw_std()                                            { if (!m_isDaemon) { redrawwin(stdscr); } }

  void                resize(int w, int h)                                    { if (!m_isDaemon) { wresize(m_window, h, w); } }
  void                resize(int x, int y, int w, int h);

  static void         resize_term(int x, int y)                               { if (!m_isDaemon) { resizeterm(y, x); } }
  static void         resize_term(std::pair<int, int> dim)                    { if (!m_isDaemon) { resizeterm(dim.second, dim.first); } }

  unsigned int        get_x()                                                 { int x, __UNUSED y; if (!m_isDaemon) { getyx(m_window, y, x); } else { x=1; } return x; }
  unsigned int        get_y()                                                 { int x, y; if (!m_isDaemon) { getyx(m_window, y, x); } else { y=1; } return y; }

  unsigned int        width()                                                 { int x, __UNUSED y; if (!m_isDaemon) { getmaxyx(m_window, y, x); } else { x=80; } return x; }
  unsigned int        height()                                                { int x, y; if (!m_isDaemon) { getmaxyx(m_window, y, x); } else { y=24; } return y; }

  void                move(unsigned int x, unsigned int y)                    { if (!m_isDaemon) { wmove(m_window, y, x); } }

  chtype              get_background()                                        { chtype bg=0; if (!m_isDaemon) { bg=getbkgd(m_window); } return bg; }
  void                set_background(chtype c)                                { if (!m_isDaemon) { return wbkgdset(m_window, c); } }

  void                erase()                                                 { if (!m_isDaemon) { werase(m_window); } }
  static void         erase_std()                                             { if (!m_isDaemon) { werase(stdscr); } }

  void                print_border(chtype ls, chtype rs,
                                   chtype ts, chtype bs,
                                   chtype tl, chtype tr,
                                   chtype bl, chtype br)                      { if (!m_isDaemon) { wborder(m_window, ls, rs, ts, bs, tl, tr, bl, br); } }

  // The format string is non-const, but that will not be a problem
  // since the string shall always be a C string choosen at
  // compiletime. Might cause extra copying of the string?

  void                print(const char* str, ...);
  void                print(unsigned int x, unsigned int y, const char* str, ...);

  void                print_attributes(unsigned int x, unsigned int y, const char* first, const char* last, const attributes_list* attributes);

  void                print_char(const chtype ch)                                 { if (!m_isDaemon) { waddch(m_window, ch); } }
  void                print_char(unsigned int x, unsigned int y, const chtype ch) { if (!m_isDaemon) { mvwaddch(m_window, y, x, ch); } }

  void                set_attr(unsigned int x, unsigned int y, unsigned int n, int attr, int color) { if (!m_isDaemon) { mvwchgat(m_window, y, x, n, attr, color, NULL); } }

  void                set_default_attributes(int attr)                            { if (!m_isDaemon) { (void)wattrset(m_window, attr); } }

  // Initialize stdscr.
  static void         initialize();
  static void         cleanup();

  static int          get_screen_width()                                      { int x, __UNUSED y; if (!m_isDaemon) { getmaxyx(stdscr, y, x); } else { x=80; } return x; }
  static int          get_screen_height()                                     { int x, y; if (!m_isDaemon) { getmaxyx(stdscr, y, x); } else { y=24;} return y; }

  static std::pair<int, int> term_size();

  static void         do_update()                                             { if (!m_isDaemon) { doupdate(); } }

  static bool         daemon()                                                { return m_isDaemon; }

private:
  Canvas(const Canvas&);
  void operator = (const Canvas&);

  static bool         m_isInitialized;
  static bool         m_isDaemon;

  WINDOW*             m_window;
};

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

}

#endif
