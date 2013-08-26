// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <stdexcept>

#include "core/curl_get.h"
#include "core/http_queue.h"

#include "canvas.h"
#include "rak/functional.h"
#include "window_http_queue.h"

namespace display {

WindowHttpQueue::WindowHttpQueue(core::HttpQueue* q) :
  Window(new Canvas, 0, 0, 1, extent_full, 1),
  m_queue(q) {
  
  set_active(false);
  m_connInsert = m_queue->signal_insert().insert(m_queue->signal_insert().end(),
                                                 std::tr1::bind(&WindowHttpQueue::receive_insert,
                                                                this,
                                                                std::tr1::placeholders::_1));
  m_connErase  = m_queue->signal_erase().insert(m_queue->signal_insert().end(),
                                                std::tr1::bind(&WindowHttpQueue::receive_erase,
                                                               this,
                                                               std::tr1::placeholders::_1));
}

void
WindowHttpQueue::redraw() {
  m_slotSchedule(this, (cachedTime + rak::timer::from_seconds(1)).round_seconds());

  cleanup_list();

  if (m_container.empty()) {
    set_active(false);
    m_slotAdjust();

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
    if (itr->m_http == NULL && itr->m_timer < cachedTime)
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
    m_slotAdjust();
  }
  
  mark_dirty();
}

void
WindowHttpQueue::receive_erase(core::CurlGet* h) {
  Container::iterator itr = std::find_if(m_container.begin(), m_container.end(), rak::equal(h, std::mem_fun_ref(&Node::get_http)));

  if (itr == m_container.end())
    throw std::logic_error("WindowHttpQueue::receive_erase(...) tried to remove an object we don't have");

  itr->m_http = NULL;
  itr->m_timer = cachedTime + rak::timer::from_seconds(1);

  mark_dirty();
}

}
