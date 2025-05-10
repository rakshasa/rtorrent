#include "config.h"

#include <stdexcept>
#include <algorithm>

#include "canvas.h"
#include "globals.h"
#include "manager.h"
#include "window.h"

namespace display {

Manager::Manager() {
  m_scheduler.external_set_thread_id(std::this_thread::get_id());

  m_task_update.slot() = std::bind(&Manager::receive_update, this);
}

Manager::~Manager() {
  torrent::this_thread::scheduler()->erase(&m_task_update);
}

void
Manager::force_redraw() {
  m_force_redraw = true;
}

void
Manager::schedule(Window* w, std::chrono::microseconds t) {
  m_scheduler.external_set_cached_time(torrent::this_thread::cached_time());
  m_scheduler.update_wait_until(w->task_update(), t);

  schedule_update(50ms);
}

void
Manager::unschedule(Window* w) {
  m_scheduler.external_set_cached_time(torrent::this_thread::cached_time());
  m_scheduler.erase(w->task_update());

  schedule_update(50ms);
}

void
Manager::adjust_layout() {
  Canvas::redraw_std();
  m_root_frame.balance(0, 0, Canvas::get_screen_width(), Canvas::get_screen_height());

  schedule_update(0ms);
}

void
Manager::receive_update() {
  if (m_force_redraw) {
    m_force_redraw = false;

    display::Canvas::resize_term(display::Canvas::term_size());
    Canvas::redraw_std();

    adjust_layout();
    m_root_frame.redraw();
  }

  Canvas::refresh_std();

  m_scheduler.external_perform(torrent::this_thread::cached_time());
  m_root_frame.refresh();

  Canvas::do_update();

  m_time_last_update = torrent::this_thread::cached_time();
  schedule_update(50ms);
}

void
Manager::schedule_update(std::chrono::microseconds min_interval) {
  if (m_scheduler.empty()) {
    torrent::this_thread::scheduler()->erase(&m_task_update);
    return;
  }

  if (m_task_update.is_scheduled() && m_task_update.time() <= m_scheduler.front()->time())
    return;

  torrent::this_thread::scheduler()->update_wait_until(&m_task_update, std::max(m_scheduler.front()->time(), m_time_last_update + min_interval));
}

}
