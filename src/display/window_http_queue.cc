#include "config.h"

#include "display/window_http_queue.h"

#include <stdexcept>

#include "core/curl_get.h"
#include "core/http_queue.h"
#include "display/canvas.h"

namespace display {

WindowHttpQueue::WindowHttpQueue(core::HttpQueue* q) :
  Window(new Canvas, 0, 0, 1, extent_full, 1),
  m_queue(q) {

  set_active(false);
  m_connInsert = m_queue->signal_insert().insert(m_queue->signal_insert().end(),
                                                 std::bind(&WindowHttpQueue::receive_insert,
                                                                this,
                                                                std::placeholders::_1));
  m_connErase  = m_queue->signal_erase().insert(m_queue->signal_insert().end(),
                                                std::bind(&WindowHttpQueue::receive_erase,
                                                               this,
                                                               std::placeholders::_1));
}

void
WindowHttpQueue::redraw() {
  schedule_update();
  cleanup_list();

  if (m_container.empty()) {
    set_active(false);
    m_slot_adjust();

    return;
  }

  m_canvas->erase();
  m_canvas->print(0, 0, "Http [%i]", m_queue->size());

  unsigned int pos = 10;
  Container::iterator itr = m_container.begin();

  while (itr != m_container.end() && pos + 20 < m_canvas->width()) {
    if (itr->m_http == NULL)
      m_canvas->print(pos, 0, "%s done", itr->m_name.c_str());

    else if (itr->m_http->size_total() == 0)
      m_canvas->print(pos, 0, "%s ---%%", itr->m_name.c_str());

    else
      m_canvas->print(pos, 0, "%s %3i%%", itr->m_name.c_str(), (int)(100.0 * itr->m_http->size_done() / itr->m_http->size_total()));

    pos += itr->m_name.size() + 6;
    ++itr;
  }
}

void
WindowHttpQueue::cleanup_list() {
  for (Container::iterator itr = m_container.begin(); itr != m_container.end();)
    if (itr->m_http == NULL && itr->m_timer < torrent::this_thread::cached_time())
      itr = m_container.erase(itr);
    else
      ++itr;

  // Bad, can't have this here as it is called from redraw().
  //   mark_dirty();
}

std::string
WindowHttpQueue::create_name(core::CurlGet* h) {
  size_t p = h->url().rfind('/', h->url().size() - std::min<int>(10, h->url().size()));

  std::string n = p != std::string::npos ? h->url().substr(p) : h->url();

  if (n.empty())
    throw std::logic_error("WindowHttpQueue::create_name(...) made a bad string");

  if (n.size() > 2 && n[0] == '/')
    n = n.substr(1);

  if (n.size() > 9 &&
      (n.substr(n.size() - 8) == ".torrent" ||
       n.substr(n.size() - 8) == ".TORRENT"))
    n = n.substr(0, n.size() - 8);

  if (n.size() > 30)
    n = n.substr(0, 30);

  return n;
}

void
WindowHttpQueue::receive_insert(core::CurlGet* h) {
  m_container.push_back(Node(h, create_name(h)));

  if (!is_active()) {
    set_active(true);
    m_slot_adjust();
  }

  mark_dirty();
}

void
WindowHttpQueue::receive_erase(core::CurlGet* h) {
  Container::iterator itr = std::find_if(m_container.begin(), m_container.end(), [h](Node n) { return h == n.get_http(); });

  if (itr == m_container.end())
    throw std::logic_error("WindowHttpQueue::receive_erase(...) tried to remove an object we don't have");

  itr->m_http = NULL;
  itr->m_timer = torrent::this_thread::cached_time() + 1s;

  mark_dirty();
}

}
