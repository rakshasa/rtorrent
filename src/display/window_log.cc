#include "config.h"

#include "core/log.h"

#include "canvas.h"
#include "window_log.h"

namespace display {

WindowLog::WindowLog(core::Log* l) :
  Window(new Canvas, false, 0),
  m_log(l) {

  set_active(false);

  m_connUpdate = l->signal_update().connect(sigc::mem_fun(*this, &WindowLog::receive_update));
}

WindowLog::~WindowLog() {
  m_connUpdate.disconnect();
}

void
WindowLog::redraw() {
  if (!is_dirty())
    return;

  m_lastDraw = utils::Timer::cache();

  m_canvas->erase();

  int pos = 0;

  for (core::Log::iterator itr = m_log->begin(), end = m_log->end(); itr != end; ++itr)
    m_canvas->print(0, pos++, "Log: %s", itr->second.c_str());
}

void
WindowLog::receive_update() {
  int newHeight = std::min<size_t>(m_log->size(), 5);

  if (newHeight != m_minHeight) {
    m_minHeight = newHeight;

    set_active(m_minHeight != 0);
    m_slotAdjust();
  }

  mark_dirty();
}

}
