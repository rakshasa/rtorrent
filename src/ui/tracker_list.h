#ifndef RTORRENT_UI_TRACKER_LIST_H
#define RTORRENT_UI_TRACKER_LIST_H

#include "core/download.h"
#include "display/manager.h"
#include "input/bindings.h"

namespace display {
  class WindowTrackerList;
}

namespace ui {

class Control;

class TrackerList {
public:
  typedef display::WindowTrackerList    WTrackerList;
  typedef display::Manager::iterator    MItr;

  TrackerList(Control* c, core::Download* d);

  void                activate(MItr mItr);
  void                disable();

  input::Bindings&    get_bindings() { return m_bindings; }

private:
  void                receive_next();
  void                receive_prev();

  core::Download*     m_download;
  WTrackerList*       m_window;
  
  Control*            m_control;
  input::Bindings     m_bindings;

  // Change to unsigned, please.
  unsigned int        m_focus;
};

}

#endif
