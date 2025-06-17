#include "config.h"

#include "display/window_http_queue.h"

#include <stdexcept>
#include <torrent/net/http_get.h>

#include "core/http_queue.h"
#include "display/canvas.h"

namespace display {

// TODO: Large downloads result in rtorrent not quiting on ^Q.

WindowHttpQueue::WindowHttpQueue(core::HttpQueue* q) :
  Window(new Canvas, 0, 0, 1, extent_full, 1),
  m_queue(q) {

  set_active(false);

  m_conn_insert = m_queue->signal_insert().insert(m_queue->signal_insert().end(), [this](auto h) { receive_insert(h); });
  m_conn_erase  = m_queue->signal_erase().insert(m_queue->signal_insert().end(), [this](auto h) { receive_erase(h); });

  m_task_deactivate.slot() = [this] {
      if (!m_container.empty())
        return;

      set_active(false);
      m_slot_adjust();

      mark_dirty();
    };
}

WindowHttpQueue::~WindowHttpQueue() {
  m_queue->signal_insert().erase(m_conn_insert);
  m_queue->signal_erase().erase(m_conn_erase);

  torrent::this_thread::scheduler()->erase(&m_task_deactivate);
}

void
WindowHttpQueue::redraw() {
  bool is_empty = m_container.empty();

  schedule_update();
  cleanup_list();

  if (!is_empty && m_container.empty())
    torrent::this_thread::scheduler()->update_wait_for(&m_task_deactivate, 5s);

  m_canvas->erase();
  m_canvas->print(0, 0, "Http [%i]", m_queue->size());

  unsigned int pos = 10;
  Container::iterator itr = m_container.begin();

  while (itr != m_container.end() && pos + 10 < m_canvas->width()) {
    if (!itr->m_http.is_valid())
      m_canvas->print(pos, 0, "[%s done]", itr->m_name.c_str());

    else if (itr->m_http.size_total() == 0)
      m_canvas->print(pos, 0, "[%s ---%%]", itr->m_name.c_str());

    else
      m_canvas->print(pos, 0, "[%s %3i%%]", itr->m_name.c_str(), (int)(100.0 * itr->m_http.size_done() / itr->m_http.size_total()));

    pos += itr->m_name.size() + 6;
    ++itr;
  }
}

void
WindowHttpQueue::cleanup_list() {
  for (Container::iterator itr = m_container.begin(); itr != m_container.end(); ) {
    if (!itr->m_http.is_valid() && itr->m_timer < torrent::this_thread::cached_time()) {
      itr = m_container.erase(itr);
      continue;
    }

    ++itr;
  }
}

std::string
WindowHttpQueue::create_name(torrent::net::HttpGet http_get) {
  size_t p = http_get.url().rfind('/', http_get.url().size() - std::min<int>(10, http_get.url().size()));

  std::string n = p != std::string::npos ? http_get.url().substr(p) : http_get.url();

  if (n.empty())
    throw std::logic_error("WindowHttpQueue::create_name(...) made a bad string");

  if (n.size() > 2 && n[0] == '/')
    n = n.substr(1);

  if (n.size() > 9 &&
      (n.substr(n.size() - 8) == ".torrent" ||
       n.substr(n.size() - 8) == ".TORRENT"))
    n = n.substr(0, n.size() - 8);

  if (n.size() > 20)
    n = n.substr(0, 20);

  return n;
}

void
WindowHttpQueue::receive_insert(torrent::net::HttpGet http_get) {
  m_container.push_back(Node(http_get, create_name(http_get)));

  if (!is_active()) {
    set_active(true);
    m_slot_adjust();
  }

  mark_dirty();
}

void
WindowHttpQueue::receive_erase(torrent::net::HttpGet http_get) {
  auto itr = std::find_if(m_container.begin(),
                          m_container.end(),
                          [http_get](auto& n) { return http_get == n.m_http; });

  if (itr == m_container.end())
    throw std::logic_error("WindowHttpQueue::receive_erase(...) tried to remove an object we don't have");

  itr->m_http = torrent::net::HttpGet();
  itr->m_timer = torrent::this_thread::cached_time() + 4s;

  mark_dirty();
}

}
