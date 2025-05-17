#ifndef RTORRENT_NAVIGATION_KEYMAP_H
#define RTORRENT_NAVIGATION_KEYMAP_H

#include <map>

#include <curses.h>

namespace input {

enum NavigationKeymap {
  RT_KEY_LEFT,
  RT_KEY_RIGHT,
  RT_KEY_UP,
  RT_KEY_DOWN,
  RT_KEY_DISPLAY_LOG,
  RT_KEY_DISCONNECT_PEER,
  RT_KEY_PPAGE,
  RT_KEY_NPAGE,
  RT_KEY_HOME,
  RT_KEY_END,
  RT_KEY_TOGGLE_LAYOUT,
  RT_KEYMAP_MAX,
};

static const int emacs_keymap[RT_KEYMAP_MAX] = {
  'B' - '@',
  'F' - '@',
  'P' - '@',
  'N' - '@',
  'l',
  'k',
  'U' - '@',
  'H' - '@',
  'A' - '@',
  'E' - '@',
  'L'
};

static const int vi_keymap[RT_KEYMAP_MAX] = {
  'h',
  'l',
  'k',
  'j',
  'L',
  'K',
  'B' - '@',
  'D' - '@',
  'H',
  'G',
  'L' - '@'
};

} // namespace display
#endif
