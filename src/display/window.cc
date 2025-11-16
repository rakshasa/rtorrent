#include "config.h"

#include "window.h"

#include <stdexcept>

#include <torrent/utils/chrono.h>

namespace display {

Window::SlotTimer  Window::m_slot_schedule;
Window::SlotWindow Window::m_slot_unschedule;
Window::Slot       Window::m_slot_adjust;

// When constructing the window we set flag_offscreen so that redraw
// doesn't get triggered until after a successful Frame::balance call.

Window::Window(Canvas* canvas, int flags, extent_type min_width, extent_type min_height, extent_type max_width, extent_type max_height) :
  m_canvas(canvas),
  m_flags(flags | flag_offscreen),

  m_min_width(min_width),
  m_min_height(min_height),

  m_max_width(max_width),
  m_max_height(max_height) {

  m_task_update.slot() = [this] { redraw(); };
}

Window::~Window() {
  if (is_active())
    m_slot_unschedule(this);
}

void
Window::set_active(bool state) {
  if (state == is_active())
    return;

  if (state) {
    // Set offscreen so we don't try rendering before Frame::balance
    // has been called.
    m_flags |= flag_active | flag_offscreen;
    mark_dirty();

  } else {
    m_flags &= ~flag_active;
    m_slot_unschedule(this);
  }
}

void
Window::resize(int x, int y, int w, int h) {
  if (x < 0 || y < 0)
    throw std::logic_error("Window::resize(...) bad x or y position");

  if (w <= 0 || h <= 0)
    throw std::logic_error("Window::resize(...) bad size");

  m_canvas->resize(x, y, w, h);
}

void
Window::schedule_update(unsigned int wait_seconds) {
  if (wait_seconds == 0) {
    m_slot_schedule(this, torrent::utils::ceil_seconds(torrent::this_thread::cached_time() + 100ms));
    return;
  }

  m_slot_schedule(this, torrent::utils::ceil_seconds(torrent::this_thread::cached_time() + 1s*wait_seconds));
}

}
