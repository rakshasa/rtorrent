#include "config.h"

#include "display/color_map.h"
#include <rak/algorithm.h>

#include "core/download.h"
#include "core/view.h"
#include "rpc/parse_commands.h"

#include "canvas.h"
#include "globals.h"
#include "utils.h"
#include "window_download_list.h"

namespace display {

WindowDownloadList::WindowDownloadList() :
    Window(new Canvas, 0, 120, 1, extent_full, extent_full) {
}

WindowDownloadList::~WindowDownloadList() {
  if (m_view != NULL)
    m_view->signal_changed().erase(m_changed_itr);

  m_view = NULL;
}

void
WindowDownloadList::set_view(core::View* l) {
  if (m_view != NULL)
    m_view->signal_changed().erase(m_changed_itr);

  m_view = l;

  if (m_view != NULL)
    m_changed_itr = m_view->signal_changed().insert(m_view->signal_changed().begin(), std::bind(&Window::mark_dirty, this));
}

// Return a pair of ints, representing a) the ncurses attributes and b) the ncurses color pair ID to use
std::pair<int, int>
WindowDownloadList::get_attr_color(core::View::iterator selected) {
  core::Download* item       = *selected;
  unsigned long   focus_attr = selected == m_view->focus() ? m_canvas->attr_map().at(RCOLOR_FOCUS) : 0;
  int             offset     = (((selected - m_view->begin_visible()) & 1) + 1) * RCOLOR_MAX; // Determine the even/odd offset for the color pair
  bool            active     = item->is_open() && item->is_active();
  int             title_color;
  if (item->is_done())
    title_color = (active ? item->info()->up_rate()->rate() ? RCOLOR_SEEDING : RCOLOR_COMPLETE : RCOLOR_STOPPED) + offset;
  else
    title_color = (active ? item->info()->down_rate()->rate() ? RCOLOR_LEECHING : RCOLOR_INCOMPLETE : RCOLOR_QUEUED) + offset;
  return std::make_pair(m_canvas->attr_map().at(title_color) | focus_attr, title_color);
}

int
WindowDownloadList::page_size(const std::string layout_name) {
  // Calculate the page size for torrents. This is a public method
  // because it's also used to determine the default size for page
  // up/down actions.
  int layout_height;
  if (layout_name == "full") {
    layout_height = 3;
  } else if (layout_name == "compact") {
    layout_height = 1;
  } else {
    return 0;
  }
  return m_canvas->height() / layout_height;
}

int
WindowDownloadList::page_size() {
  return page_size(rpc::call_command_string("ui.torrent_list.layout"));
}

void
WindowDownloadList::redraw() {
  if (m_canvas->daemon())
    return;

  schedule_update();

  m_canvas->erase();

  if (m_view == NULL)
    return;

  m_canvas->print(0, 0, "%s", ("[View: " + m_view->name() + (m_view->get_filter_temp().is_empty() ? "" : " (filtered)") + "]").c_str());

  if (m_view->empty_visible() || m_canvas->width() < 5 || m_canvas->height() < 2)
    return;

  // show "X of Y"
  if (m_canvas->width() > 16 + 8 + m_view->name().length()) {
    int item_idx = m_view->focus() - m_view->begin_visible();
    if (item_idx == int(m_view->size()))
      m_canvas->print(m_canvas->width() - 16, 0, "[ none of %-5d]", m_view->size());
    else
      m_canvas->print(m_canvas->width() - 16, 0, "[%5d of %-5d]", item_idx + 1, m_view->size());
  }

  m_canvas->set_attr(0, 0, -1, RCOLOR_TITLE);

  const std::string layout_name = rpc::call_command_string("ui.torrent_list.layout");

  typedef std::pair<core::View::iterator, core::View::iterator> Range;

  Range range = rak::advance_bidirectional(m_view->begin_visible(),
                                           m_view->focus() != m_view->end_visible() ? m_view->focus() : m_view->begin_visible(),
                                           m_view->end_visible(),
                                           page_size(layout_name));

  // Make sure we properly fill out the last lines so it looks like
  // there are more torrents, yet don't hide it if we got the last one
  // in focus.
  if (range.second != m_view->end_visible())
    ++range.second;

  int   pos = 1;
  std::string buffer(m_canvas->width() + 1, ' ');
  char* last = buffer.data() + m_canvas->width() - 2 + 1;

  // Add a proper 'column info' method.
  if (layout_name == "compact") {
    print_download_column_compact(buffer.data(), last);

    m_canvas->set_default_attributes(A_BOLD);
    m_canvas->print(0, pos++, "  %s", buffer.data());
  }

  if (layout_name == "full") {
    while (range.first != range.second) {
      bool      is_focused  = range.first == m_view->focus();
      char      focus_char  = is_focused ? '*' : ' ';
      ColorKind focus_color = is_focused ? RCOLOR_FOCUS : RCOLOR_LABEL;
      auto      attr_color  = get_attr_color(range.first);

      print_download_title(buffer.data(), last, *range.first);
      m_canvas->print(0, pos, "%c %s", focus_char, buffer.data());
      m_canvas->set_attr(2, pos++, -1, attr_color.first, attr_color.second);

      print_download_info_full(buffer.data(), last, *range.first);
      m_canvas->print(0, pos, "%c %s", focus_char, buffer.data());
      m_canvas->set_attr(2, pos++, -1, focus_color);

      print_download_status(buffer.data(), last, *range.first);
      m_canvas->print(0, pos, "%c %s", focus_char, buffer.data());
      m_canvas->set_attr(2, pos++, -1, focus_color);

      range.first++;
    }

  } else {
    while (range.first != range.second) {
      char focus_char = range.first == m_view->focus() ? '*' : ' ';
      auto attr_color = get_attr_color(range.first);

      print_download_info_compact(buffer.data(), last, *range.first);
      m_canvas->print(0, pos, "%c %s", focus_char, buffer.data());
      m_canvas->set_attr(2, pos++, -1, attr_color.first, attr_color.second);

      range.first++;
    }
  }
}

} // namespace display
