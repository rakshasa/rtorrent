#include "config.h"

#include <ctime>

#include "canvas.h"
#include "window_log_complete.h"

namespace display {

WindowLogComplete::WindowLogComplete(core::Log* l) :
  Window(new Canvas, true),
  m_log(l) {

  // We're trying out scheduled tasks instead.
  m_connUpdate = l->signal_update().connect(sigc::mem_fun(*this, &WindowLogComplete::receive_update));
}

WindowLogComplete::~WindowLogComplete() {
  m_connUpdate.disconnect();
}

WindowLogComplete::iterator
WindowLogComplete::find_older() {
  return m_log->find_older(utils::Timer::cache() - 60*1000000);
}

void
WindowLogComplete::redraw() {
  m_nextDraw = utils::Timer::max();

  m_canvas->erase();

  int pos = 0;

  m_canvas->print(std::max(0, (int)m_canvas->get_width() / 2 - 5), pos++, "*** Log ***");

  for (core::Log::iterator itr = m_log->begin(), e = m_log->end(); itr != e && pos < m_canvas->get_height(); ++itr) {
    std::tm *t = std::localtime(&itr->first.tval().tv_sec);

    if (t == NULL)
      m_canvas->print(0, pos++, "(time error) %s",
		      itr->second.c_str());
    else
      m_canvas->print(0, pos++, "(%02i:%02i:%02i) %s",
		      t->tm_hour, t->tm_min, t->tm_sec,
		      itr->second.c_str());
  }
}

void
WindowLogComplete::receive_update() {
  mark_dirty();
}

}
