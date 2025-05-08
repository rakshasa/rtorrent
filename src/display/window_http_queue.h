#ifndef RTORRENT_DISPLAY_WINDOW_HTTP_QUEUE_H
#define RTORRENT_DISPLAY_WINDOW_HTTP_QUEUE_H

#include <functional>
#include <list>

#include "window.h"

namespace core {
  class CurlGet;
  class HttpQueue;
}

namespace display {

class WindowHttpQueue : public Window {
public:
  typedef std::function<void (core::CurlGet*)> slot_curl_get;
  typedef std::list<slot_curl_get>             signal_curl_get;

  WindowHttpQueue(core::HttpQueue* q);
  ~WindowHttpQueue() override;

  void                redraw() override;

private:
  struct Node {
    Node(core::CurlGet* h, const std::string& n) : m_http(h), m_name(n) {}

    core::CurlGet*            m_http{};
    std::string               m_name;
    std::chrono::microseconds m_timer{};
  };

  typedef std::list<Node> Container;

  void                cleanup_list();

  void                receive_insert(core::CurlGet* h);
  void                receive_erase(core::CurlGet* h);

  static std::string  create_name(core::CurlGet* h);

  core::HttpQueue*    m_queue;
  Container           m_container;

  signal_curl_get::iterator m_conn_insert;
  signal_curl_get::iterator m_conn_erase;

  torrent::utils::SchedulerEntry m_task_deactivate;
};

}

#endif
