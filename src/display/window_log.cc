#include "config.h"

#include <ctime>
#include <mutex>
#include <vector>
#include <torrent/torrent.h>
#include <torrent/system/callbacks.h>
#include <torrent/system/thread.h>

#include "canvas.h"
#include "utils.h"
#include "window_log.h"

namespace display {

WindowLog::WindowLog(torrent::log_buffer* l) :
  Window(new Canvas, 0, 0, 0, extent_full, extent_static),
  m_log(l) {

  m_task_update.slot() = [this]() { receive_update(); };

  m_log->lock_and_set_update_slot([this]() {
      if (m_log_updating.exchange(true) == true)
        return;

      torrent::main_thread::callback([this]() { receive_update(); });
    });
}

WindowLog::~WindowLog() {
  m_log->lock_and_set_update_slot(nullptr);
  torrent::this_thread::scheduler()->erase(&m_task_update);
}

WindowLog::iterator
WindowLog::find_older() {
  return m_log->find_older(torrent::this_thread::cached_seconds().count() - 60);
}

void
WindowLog::redraw() {
  m_canvas->erase();

  int pos = m_canvas->height();

  std::vector<torrent::log_entry> entries;

  {
    std::lock_guard<torrent::log_buffer> guard(*m_log);

    for (iterator itr = m_log->end(), last = find_older(); itr != last && entries.size() < static_cast<size_t>(pos); )
      entries.push_back(*--itr);
  }

  for (const auto& entry : entries) {
    char buffer[16];
    print_hhmmss_local(buffer, buffer + 16, static_cast<time_t>(entry.timestamp));

    m_canvas->print(0, --pos, "(%s) %s", buffer, entry.message.c_str());
  }
}

// When WindowLog is activated, call receive_update() to ensure it
// gets updated.
void
WindowLog::receive_update() {
  m_log_updating = false;

  if (!is_active())
    return;

  std::iterator_traits<iterator>::difference_type height;

  {
    std::lock_guard<torrent::log_buffer> guard(*m_log);

    height = std::min(std::distance(find_older(), (iterator)m_log->end()), (std::iterator_traits<iterator>::difference_type)10);
  }

  if (height != m_max_height) {
    m_min_height = height != 0 ? 1 : 0;
    m_max_height = height;
    mark_dirty();
    m_slot_adjust();

  } else {
    mark_dirty();
  }

  if (height == 0) {
    torrent::this_thread::scheduler()->erase(&m_task_update);
    return;
  }

  torrent::this_thread::scheduler()->update_wait_for_ceil_seconds(&m_task_update, 5s);
}

}
