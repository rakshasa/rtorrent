#ifndef RTORRENT_DISPLAY_WINDOW_LOG_H
#define RTORRENT_DISPLAY_WINDOW_LOG_H

#include <sigc++/connection.h>

#include "core/log.h"

#include "window.h"

namespace display {

class WindowLog : public Window {
public:
  typedef core::Log::iterator iterator;

  WindowLog(core::Log* l);
  ~WindowLog();

  virtual void     redraw();

  void             receive_update();

private:
  inline iterator  find_older();

  core::Log*       m_log;
  sigc::connection m_connUpdate;
};

}

#endif
