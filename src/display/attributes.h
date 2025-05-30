#ifndef RTORRENT_DISPLAY_ATTRIBUTES_H
#define RTORRENT_DISPLAY_ATTRIBUTES_H

#include <string>
#include <vector>

#if defined(HAVE_NCURSESW_CURSES_H)
#include <ncursesw/curses.h>
#elif defined(HAVE_NCURSESW_H)
#include <ncursesw.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
#include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#include <curses.h>
#else
#error "SysV or X/Open-compatible Curses header file required"
#endif

// Let us hail the creators of curses for being idiots. The only
// clever move they made was in the naming.
#undef timeout
#undef move

namespace display {

class Attributes {
public:
  static const int a_invalid = ~int();
  static const int a_normal  = A_NORMAL;
  static const int a_bold    = A_BOLD;
  static const int a_reverse = A_REVERSE;

  static const int color_invalid = ~int();
  static const int color_default = 0;

  Attributes() = default;
  Attributes(const char* pos, int attr, int col) :
    m_position(pos), m_attributes(attr), m_colors(col) {}
  Attributes(const char* pos, const Attributes& old) :
    m_position(pos), m_attributes(old.m_attributes), m_colors(old.m_colors) {}

  const char*         position() const              { return m_position; }
  void                set_position(const char* pos) { m_position = pos; }

  int                 attributes() const            { return m_attributes; }
  void                set_attributes(int attr)      { m_attributes = attr; }

  int                 colors() const                { return m_colors; }
  void                set_colors(int col)           { m_colors = col; }

private:
  const char*         m_position;
  int                 m_attributes;
  int                 m_colors;
};

}

#endif
