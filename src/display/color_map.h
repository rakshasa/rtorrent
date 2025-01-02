#ifndef RTORRENT_DISPLAY_COLOR_MAP_H
#define RTORRENT_DISPLAY_COLOR_MAP_H

#include <map>

#include <curses.h>

namespace display {

enum ColorKind {
  RCOLOR_NCURSES_DEFAULT, // Color 0 is reserved by ncurses and cannot be changed
  RCOLOR_TITLE,
  RCOLOR_FOOTER,
  RCOLOR_FOCUS,
  RCOLOR_LABEL,
  RCOLOR_INFO,
  RCOLOR_ALARM,
  RCOLOR_COMPLETE,
  RCOLOR_SEEDING,
  RCOLOR_STOPPED,
  RCOLOR_QUEUED,
  RCOLOR_INCOMPLETE,
  RCOLOR_LEECHING,
  RCOLOR_ODD,
  RCOLOR_EVEN,
  RCOLOR_MAX,
};

static const char* color_vars[RCOLOR_MAX] = {
  0,
  "ui.color.title",
  "ui.color.footer",
  "ui.color.focus",
  "ui.color.label",
  "ui.color.info",
  "ui.color.alarm",
  "ui.color.complete",
  "ui.color.seeding",
  "ui.color.stopped",
  "ui.color.queued",
  "ui.color.incomplete",
  "ui.color.leeching",
  "ui.color.odd",
  "ui.color.even",
};

} // namespace display
#endif
