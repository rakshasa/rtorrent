// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

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
