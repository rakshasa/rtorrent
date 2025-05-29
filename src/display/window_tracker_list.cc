#include "config.h"

#include "window_tracker_list.h"

#include <rak/algorithm.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/tracker/tracker.h>

#include "core/download.h"

namespace display {

WindowTrackerList::WindowTrackerList(core::Download* d, unsigned int* focus) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_full),
  m_download(d),
  m_focus(focus) {
}

void
WindowTrackerList::redraw() {
  // TODO: Make this depend on tracker signal.
  schedule_update(10);
  m_canvas->erase();

  auto pos = 0u;
  auto tc = m_download->tracker_controller();
  auto tc_size = tc.size();

  m_canvas->print(2, pos, "Trackers: [Key: %08x] [%s %s %s]",
                  tc.key(),
                  tc.is_requesting() ? "req" : "   ",
                  tc.is_promiscuous_mode() ? "prom" : "    ",
                  tc.is_failure_mode() ? "fail" : "    ");
  ++pos;

  if (tc_size == 0 || *m_focus >= tc_size)
    return;

  auto range = rak::advance_bidirectional<unsigned int>(0, *m_focus, tc_size, (m_canvas->height() - 1) / 2);
  auto group = tc.at(range.first).group();

  while (range.first != range.second) {
    auto tracker = tc.at(range.first);

    if (tracker.group() == group)
      m_canvas->print(0, pos, "%2i:", group++);

    m_canvas->print(4, pos++, "%s",
                    tracker.url().c_str());

    if (pos < m_canvas->height()) {
      const char* state;

      if (tracker.is_busy_not_scrape())
        state = "req ";
      else if (tracker.is_busy())
        state = "scr ";
      else
        state = "    ";

      auto tracker_state = tracker.state();

      m_canvas->print(0, pos++, "%s Id: %s Counters: %uf / %us (%u) %s S/L/D: %u/%u/%u (%u/%u)",
                      state,
                      rak::copy_escape_html(tracker.tracker_id()).c_str(),
                      tracker_state.failed_counter(),
                      tracker_state.success_counter(),
                      tracker_state.scrape_counter(),
                      tracker.is_usable() ? " on" : tracker.is_enabled() ? "err" : "off",
                      tracker_state.scrape_complete(),
                      tracker_state.scrape_incomplete(),
                      tracker_state.scrape_downloaded(),
                      tracker_state.latest_new_peers(),
                      tracker_state.latest_sum_peers());
    }

    if (range.first == *m_focus) {
      m_canvas->set_attr(4, pos - 2, m_canvas->width(), is_focused() ? A_REVERSE : A_BOLD, COLOR_PAIR(0));
      m_canvas->set_attr(4, pos - 1, m_canvas->width(), is_focused() ? A_REVERSE : A_BOLD, COLOR_PAIR(0));
    }

    if (tracker.is_busy()) {
      m_canvas->set_attr(0, pos - 2, 4, A_REVERSE, COLOR_PAIR(0));
      m_canvas->set_attr(0, pos - 1, 4, A_REVERSE, COLOR_PAIR(0));
    }

    range.first++;

    // If we're at the end of the range, check if we can
    // show one more line for the following tracker.
    if (range.first == range.second && pos < m_canvas->height() && range.first < tc_size)
      range.second++;
  }
}

}
