// Dummy ncurses stub header for building rtorrent without ncurses.
// Provides types, macros, and no-op function stubs so the codebase
// compiles without a real curses library. All display functions become
// no-ops; use with system.daemon.set = true.

#ifndef RTORRENT_DISPLAY_CURSES_STUB_H
#define RTORRENT_DISPLAY_CURSES_STUB_H

#include <cstdarg>
#include <cstdio>
#include <cstddef>

// Opaque window type
typedef void* WINDOW;

// Character type used by curses
typedef unsigned int chtype;
typedef unsigned int attr_t;

// The standard screen
extern WINDOW* stdscr;

// Boolean constants and return codes
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef ERR
#define ERR (-1)
#endif

// Attribute constants
#define A_NORMAL   0
#define A_STANDOUT (1U << 8)
#define A_UNDERLINE (1U << 9)
#define A_REVERSE  (1U << 10)
#define A_BLINK    (1U << 11)
#define A_DIM      (1U << 12)
#define A_BOLD     (1U << 13)

// Color support
#define COLORS 0
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

#define COLOR_PAIR(n) 0

// Key constants
#define KEY_DOWN        0x102
#define KEY_UP          0x103
#define KEY_LEFT        0x104
#define KEY_RIGHT       0x105
#define KEY_HOME        0x106
#define KEY_BACKSPACE   0x107
#define KEY_DC          0x14a
#define KEY_ENTER       0x157
#define KEY_NPAGE       0x152
#define KEY_PPAGE       0x153
#define KEY_END         0x168
#define KEY_MOUSE       0x199
#define KEY_RESIZE      0x19a

// Stubbed functions
inline WINDOW* initscr() { return nullptr; }
inline int     endwin() { return 0; }
inline WINDOW* newwin(int, int, int, int) { return nullptr; }
inline int     delwin(WINDOW*) { return 0; }
inline int     wresize(WINDOW*, int, int) { return 0; }
inline int     mvwin(WINDOW*, int, int) { return 0; }
inline int     wnoutrefresh(WINDOW*) { return 0; }
inline int     redrawwin(WINDOW*) { return 0; }
inline int     werase(WINDOW*) { return 0; }
inline int     wmove(WINDOW*, int, int) { return 0; }
inline int     vw_printw(WINDOW*, char*, va_list) { return 0; }
inline int     waddch(WINDOW*, const chtype) { return 0; }
inline int     mvwaddch(WINDOW*, int, int, const chtype) { return 0; }
inline int     mvwchgat(WINDOW*, int, int, int, attr_t, short, const void*) { return 0; }
inline int     wattrset(WINDOW*, int) { return 0; }
inline int     wattr_get(WINDOW*, attr_t*, short*, void*) { return 0; }
inline int     wattr_set(WINDOW*, attr_t, short, void*) { return 0; }
inline int     doupdate() { return 0; }
inline int     start_color() { return 0; }
inline int     use_default_colors() { return 0; }
inline int     raw() { return 0; }
inline int     noraw() { return 0; }
inline int     noecho() { return 0; }
inline int     nodelay(WINDOW*, bool) { return 0; }
inline int     keypad(WINDOW*, bool) { return 0; }
inline int     curs_set(int) { return 0; }
inline int     init_pair(short, short, short) { return 0; }
inline int     pair_content(short, short*, short*) { return 0; }
inline int     resizeterm(int, int) { return 0; }
inline int     getch() { return ERR; }

// getyx / getmaxyx are macros in real ncurses; stub them as no-ops
#define getyx(w, y, x)     do { (y) = 0; (x) = 0; } while (0)
#define getmaxyx(w, y, x)  do { (y) = 24; (x) = 80; } while (0)

#endif // RTORRENT_DISPLAY_CURSES_STUB_H
