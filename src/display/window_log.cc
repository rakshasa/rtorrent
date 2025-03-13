#include "config.h"

#include <ctime>
#include <torrent/torrent.h>
#include <torrent/utils/thread.h>

#include "canvas.h"
#include "utils.h"
#include "window_log.h"

namespace display {

WindowLog::WindowLog(torrent::log_buffer* l) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_static),
  m_log(l) {

  m_taskUpdate.slot() = std::bind(&WindowLog::receive_update, this);

  unsigned int signal_index = torrent::main_thread()->signal_bitfield()->add_signal(std::bind(&WindowLog::receive_update, this));

  m_log->lock_and_set_update_slot([signal_index]() {
    torrent::main_thread()->send_event_signal(signal_index, false);
  });
}

WindowLog::~WindowLog() {
  priority_queue_erase(&taskScheduler, &m_taskUpdate);
}

WindowLog::iterator
WindowLog::find_older() {
  return m_log->find_older(cachedTime.seconds() - 60);
  // return m_log->begin();
}

void
WindowLog::redraw() {
  m_canvas->erase();

  int pos = m_canvas->height();

  for (iterator itr = m_log->end(), last = find_older(); itr != last && pos > 0; --pos) {
    itr--;

    char buffer[16];
    print_hhmmss_local(buffer, buffer + 16, static_cast<time_t>(itr->timestamp));

    m_canvas->print(0, pos - 1, "(%s) %s", buffer, itr->message.c_str());
  }
}

// When WindowLog is activated, call receive_update() to ensure it
// gets updated.
void
WindowLog::receive_update() {
  if (!is_active())
    return;

  iterator itr = find_older();
  extent_type height = std::min(std::distance(itr, (iterator)m_log->end()), (std::iterator_traits<iterator>::difference_type)10);

  if (height != m_maxHeight) {
    m_minHeight = height != 0 ? 1 : 0;
    m_maxHeight = height;
    mark_dirty();
    m_slotAdjust();

  } else {
    mark_dirty();
  }

  priority_queue_erase(&taskScheduler, &m_taskUpdate);

  if (height != 0)
    priority_queue_insert(&taskScheduler, &m_taskUpdate, (cachedTime + rak::timer::from_seconds(5)).round_seconds());
}

}
