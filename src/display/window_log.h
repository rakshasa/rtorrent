#ifndef RTORRENT_DISPLAY_WINDOW_LOG_H
#define RTORRENT_DISPLAY_WINDOW_LOG_H

#include <sigc++/connection.h>

#include "window.h"

namespace core {
  class Log;
}

namespace display {

class WindowLog : public Window {
public:
  WindowLog(core::Log* l);
  ~WindowLog();

  virtual void     redraw();

private:
  void             receive_update();

  core::Log*       m_log;
  sigc::connection m_connUpdate;
};

}

#endif
