#include "config.h"

#include <stdexcept>

#include "canvas.h"
#include "window_peer_info.h"

namespace display {

WindowPeerInfo::WindowPeerInfo(PList* l, PList::iterator* f) :
  Window(new Canvas, true),
  m_list(l),
  m_focus(f) {
}

void
WindowPeerInfo::redraw() {
  if (Timer::cache() - m_lastDraw < 1000000)
    return;

  m_lastDraw = Timer::cache();
  m_canvas->erase();

  if (*m_focus == m_list->end())
    mvprintw(0, 0, "No focus");
  else
    mvprintw(0, 0, "Focus on: %s:%i",
	     (*m_focus)->get_dns().c_str(),
	     (*m_focus)->get_port());
}

}
