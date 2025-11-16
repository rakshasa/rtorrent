#ifndef RTORRENT_DISPLAY_MANAGER_H
#define RTORRENT_DISPLAY_MANAGER_H

#include <torrent/utils/scheduler.h>

#include "display/frame.h"

namespace display {

class Window;

class Manager {
public:
  Manager();
  ~Manager();

  void                force_redraw();

  void                schedule(Window* w, std::chrono::microseconds t);
  void                unschedule(Window* w);

  void                adjust_layout();
  void                receive_update();

  // New interface.
  Frame*              root_frame() { return &m_root_frame; }

private:
  void                schedule_update(std::chrono::microseconds min_interval);

  bool                m_force_redraw{false};
  Frame               m_root_frame;

  std::chrono::microseconds         m_time_last_update{};
  torrent::utils::ExternalScheduler m_scheduler;
  torrent::utils::SchedulerEntry    m_task_update;
};

}

#endif
