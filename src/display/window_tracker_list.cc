#include "config.h"

#include <stdexcept>

#include "core/download.h"
#include "rak/algorithm.h"

#include "window_tracker_list.h"

namespace display {

WindowTrackerList::WindowTrackerList(core::Download* d, unsigned int* focus) :
  Window(new Canvas, true),
  m_download(d),
  m_focus(focus) {
}

void
WindowTrackerList::redraw() {
  // TODO: Make this depend on tracker signal.
  m_nextDraw = utils::Timer::cache().round_seconds() + 10 * 1000000;
  m_canvas->erase();

  int pos = 0;

  m_canvas->print( 2, pos, "Trackers:");

  ++pos;

  if (m_download->get_download().get_tracker_size() == 0)
    return;

  if (*m_focus >= m_download->get_download().get_tracker_size())
    throw std::logic_error("WindowTrackerList::redraw() called on an object with a bad focus value");

  typedef std::pair<unsigned int, unsigned int> Range;

  Range range = rak::advance_bidirectional<unsigned int>(0,
							 *m_focus,
							 m_download->get_download().get_tracker_size(),
							 m_canvas->get_height());

  while (range.first != range.second) {
    torrent::Tracker t = m_download->get_download().get_tracker(range.first);

    m_canvas->print(0, pos, "%c %s",
		    range.first == *m_focus ? '*' : ' ',
		    t.get_url().c_str());

    ++range.first;
    ++pos;
  }
}

}
