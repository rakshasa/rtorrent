// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

  Canvas(int x = 0, int y = 0, int width = 0, int height = 0) :
    m_window(newwin(height, width, y, x)) {}
  ~Canvas() { delwin(m_window); }

  void                refresh()                                               { wnoutrefresh(m_window); }
  static void         refresh_std()                                           { wnoutrefresh(stdscr); }
  void                redraw()                                                { redrawwin(m_window); }
  static void         redraw_std()                                            { redrawwin(stdscr); }

  void                resize(int w, int h)                                    { wresize(m_window, h, w); }
  void                resize(int x, int y, int w, int h);

  static void         resize_term(int x, int y)                               { resizeterm(y, x); }
  static void         resize_term(std::pair<int, int> dim)                    { resizeterm(dim.second, dim.first); }

  unsigned int        get_x()                                                 { int x, y; getyx(m_window, y, x); return x; }
  unsigned int        get_y()                                                 { int x, y; getyx(m_window, y, x); return y; }

  unsigned int        width()                                                 { int x, y; getmaxyx(m_window, y, x); return x; }
  unsigned int        height()                                                { int x, y; getmaxyx(m_window, y, x); return y; }

  void                move(unsigned int x, unsigned int y)                    { wmove(m_window, y, x); }

  chtype              get_background()                                        { return getbkgd(m_window); }
  void                set_background(chtype c)                                { return wbkgdset(m_window, c); }

  void                erase()                                                 { werase(m_window); }
  static void         erase_std()                                             { werase(stdscr); }

  void                print_border(chtype ls, chtype rs,
                                   chtype ts, chtype bs,
                                   chtype tl, chtype tr,
                                   chtype bl, chtype br)                      { wborder(m_window, ls, rs, ts, bs, tl, tr, bl, br); }

  // The format string is non-const, but that will not be a problem
  // since the string shall always be a C string choosen at
  // compiletime. Might cause extra copying of the string?

  void                print(unsigned int x, unsigned int y, const char* str, ...);

  void                print_attributes(unsigned int x, unsigned int y, const char* first, const char* last, const attributes_list* attributes);

  void                print_char(const chtype ch)                                 { waddch(m_window, ch); }
  void                print_char(unsigned int x, unsigned int y, const chtype ch) { mvwaddch(m_window, y, x, ch); }

  void                set_attr(unsigned int x, unsigned int y, unsigned int n, int attr, int color) { mvwchgat(m_window, y, x, n, attr, color, NULL); }

  // Initialize stdscr.
  static void         initialize();
  static void         cleanup();

  static int          get_screen_width()                                      { int x, y; getmaxyx(stdscr, y, x); return x; }
  static int          get_screen_height()                                     { int x, y; getmaxyx(stdscr, y, x); return y; }

  static std::pair<int, int> term_size();

  static void         do_update()                                             { doupdate(); }

private:
  Canvas(const Canvas&);
  void operator = (const Canvas&);

  static bool         m_isInitialized;

  WINDOW*             m_window;
};

inline void
Canvas::print(unsigned int x, unsigned int y, const char* str, ...) {
  va_list arglist;

  va_start(arglist, str);
  wmove(m_window, y, x);
  vw_printw(m_window, str, arglist);
  va_end(arglist);
}

}

#endif
