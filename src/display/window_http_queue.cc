#include "config.h"

#include <stdexcept>

#include "core/curl_get.h"
#include "core/http_queue.h"

#include "canvas.h"
#include "functional.h"
#include "window_http_queue.h"

namespace display {

WindowHttpQueue::WindowHttpQueue(core::HttpQueue* q) :
  Window(new Canvas, false, 1),
  m_queue(q) {
  
  set_active(false);
  m_connInsert = m_queue->signal_insert().connect(sigc::mem_fun(*this, &WindowHttpQueue::receive_insert));
  m_connErase  = m_queue->signal_erase().connect(sigc::mem_fun(*this, &WindowHttpQueue::receive_erase));
}

void
WindowHttpQueue::redraw() {
  if (Timer::cache() - m_lastDraw < 1000000)
    return;

  m_lastDraw = Timer::cache();

  cleanup_list();

  if (m_container.empty()) {
    set_active(false);
    m_slotAdjust();

    return;
  } 

  m_canvas->erase();
  m_canvas->print(0, 0, "Http [%i]", m_queue->size());

  int pos = 10;
  Container::iterator itr = m_container.begin();

  while (itr != m_container.end() && pos + 20 < m_canvas->get_width()) {
    if (itr->m_http == NULL)
      m_canvas->print(pos, 0, "%s done", itr->m_name.c_str());

    else if (itr->m_http->get_size_total() == 0)
      m_canvas->print(pos, 0, "%s ---%%", itr->m_name.c_str());

    else
      m_canvas->print(pos, 0, "%s %3i%%",
		      itr->m_name.c_str(),
		      (int)(100.0 * itr->m_http->get_size_done() / itr->m_http->get_size_total()));

    pos += itr->m_name.size() + 6;
    ++itr;
  }
}

void
WindowHttpQueue::cleanup_list() {
  for (Container::iterator itr = m_container.begin(); itr != m_container.end();)
    if (itr->m_http == NULL && itr->m_timer < Timer::cache())
      itr = m_container.erase(itr);
    else
      ++itr;

  mark_dirty();
}

std::string
WindowHttpQueue::create_name(core::CurlGet* h) {
  std::string n = h->get_url().substr(h->get_url().rfind('/', h->get_url().size() - std::min((size_t)10, h->get_url().size())));

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
    m_slotAdjust();
  }
  
  mark_dirty();
}

void
WindowHttpQueue::receive_erase(core::CurlGet* h) {
  Container::iterator itr = std::find_if(m_container.begin(), m_container.end(),
					 func::equal(h, std::mem_fun_ref(&Node::get_http)));

  if (itr == m_container.end())
    throw std::logic_error("WindowHttpQueue::receive_erase(...) tried to remove an object we don't have");

  itr->m_http = NULL;
  itr->m_timer = Timer::cache() + 10000000;

  mark_dirty();
}

}
