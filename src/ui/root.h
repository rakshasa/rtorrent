#ifndef RTORRENT_UI_ROOT_H
#define RTORRENT_UI_ROOT_H

#include "input/bindings.h"

namespace ui {

class DownloadList;

class Root {
public:
  Root(Control* c) : m_shutdownReceived(false), m_control(c), m_downloadList(NULL) {}

  void            init();
  void            cleanup();

  bool            get_shutdown_received()       { return m_shutdownReceived; }
  void            set_shutdown_received(bool v) { m_shutdownReceived = v; }

private:
  void            setup_keys();

  bool            m_shutdownReceived;

  Control*        m_control;
  DownloadList*   m_downloadList;

  input::Bindings m_bindings;
};

}

#endif
