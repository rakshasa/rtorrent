// Dummy ncurses stub: provides the stdscr symbol expected by the codebase.

#include "curses_stub.h"

// Define stdscr as a null pointer; all ncurses calls are no-ops in this stub.
WINDOW* stdscr = nullptr;
