#ifndef RTORRENT_DISPLAY_WINDOW_LOG_COMPLETE_H
#define RTORRENT_DISPLAY_WINDOW_LOG_COMPLETE_H

#include <sigc++/connection.h>

#include "core/log.h"

#include "window.h"

namespace display {

class WindowLogComplete : public Window {
public:
  typedef core::Log::iterator iterator;

  WindowLogComplete(core::Log* l);
  ~WindowLogComplete();

  virtual void     redraw();

  void             receive_update();

private:
  inline iterator  find_older();

  core::Log*       m_log;
  sigc::connection m_connUpdate;
};

}

#endif
