#ifndef RTORRENT_DISPLAY_WINDOW_HTTP_QUEUE_H
#define RTORRENT_DISPLAY_WINDOW_HTTP_QUEUE_H

#include <sigc++/connection.h>

#include "window.h"

namespace core {
  class CurlGet;
  class HttpQueue;
}

namespace display {

class WindowHttpQueue : public Window {
public:
  WindowHttpQueue(core::HttpQueue* q);
  ~WindowHttpQueue() { m_connInsert.disconnect(); m_connErase.disconnect(); }

  virtual void        redraw();

private:
  struct Node {
    Node(core::CurlGet* h, const std::string& n) : m_http(h), m_name(n) {}

    core::CurlGet* get_http() { return m_http; }

    core::CurlGet* m_http;
    std::string    m_name;
    utils::Timer          m_timer;
  };

  typedef std::list<Node> Container;

  void                cleanup_list();

  void                receive_insert(core::CurlGet* h);
  void                receive_erase(core::CurlGet* h);

  static std::string  create_name(core::CurlGet* h);

  core::HttpQueue*    m_queue;
  sigc::connection    m_connInsert;
  sigc::connection    m_connErase;

  Container           m_container;
};

}

#endif
