#ifndef RTORRENT_DISPLAY_WINDOW_LOG_H
#define RTORRENT_DISPLAY_WINDOW_LOG_H

#include <torrent/utils/log_buffer.h>
#include <torrent/utils/scheduler.h>

#include "window.h"

namespace display {

class WindowLog : public Window {
public:
  typedef torrent::log_buffer::const_iterator iterator;

  WindowLog(torrent::log_buffer* l);
  ~WindowLog();

  virtual void        redraw();

  void                receive_update();

private:
  inline iterator     find_older();

  torrent::log_buffer*           m_log;
  torrent::utils::SchedulerEntry m_task_update;
};

}

#endif
