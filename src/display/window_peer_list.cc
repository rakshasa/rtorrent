#include "config.h"

#include <stdexcept>

#include "core/download.h"

#include "canvas.h"
#include "window_peer_list.h"

namespace display {

WindowPeerList::WindowPeerList(core::Download* d, PList* l, PList::iterator* f) :
  Window(new Canvas, true),
  m_download(d),
  m_list(l),
  m_focus(f) {
}

void
WindowPeerList::redraw() {
  m_nextDraw = utils::Timer::cache().round_seconds() + 1000000;
  m_canvas->erase();

  int x = 2;
  int y = 0;

  m_canvas->print(x, y, "DNS");   x += 16;
  m_canvas->print(x, y, "UP");    x += 7;
  m_canvas->print(x, y, "DOWN");  x += 7;
  m_canvas->print(x, y, "PEER");  x += 7;
  m_canvas->print(x, y, "RE/LO"); x += 7;
  m_canvas->print(x, y, "QS");    x += 6;
  m_canvas->print(x, y, "DONE");  x += 6;
  m_canvas->print(x, y, "REQ");   x += 6;
  m_canvas->print(x, y, "SNUB");

  ++y;

  if (m_list->empty())
    return;

  PList::iterator itr, last;
  
  if (*m_focus != m_list->end()) {
    int i = 0;
    itr = last = *m_focus;
    
    while (i < m_canvas->get_height() - y && !(itr == m_list->begin() && last == m_list->end())) {
      if (itr != m_list->begin()) {
	--itr;
	++i;
      }
      
      if (last != m_list->end() && i < m_canvas->get_height() - y) {
	++last;
	++i;
      }
    }

  } else {
    itr = m_list->begin();
    last = m_list->end();
  }    

  if (m_download->get_download().get_chunks_total() <= 0)
    throw std::logic_error("WindowPeerList::redraw() m_slotChunksTotal() returned invalid value");

  while (itr != m_list->end() && y < m_canvas->get_height()) {
    x = 0;

    m_canvas->print(x, y, "%c %s",
	     itr == *m_focus ? '*' : ' ',
	     itr->get_dns().c_str());
    x += 18;

    m_canvas->print(x, y, "%.1f",
	     (double)itr->get_rate_up() / 1024);
    x += 7;

    m_canvas->print(x, y, "%.1f",
	     (double)itr->get_rate_down() / 1024);
    x += 7;

    m_canvas->print(x, y, "%.1f",
	     (double)itr->get_rate_peer() / 1024);
    x += 7;

    m_canvas->print(x, y, "%c%c/%c%c%c",
	     itr->get_remote_choked() ? 'c' : 'u',
	     itr->get_remote_interested() ? 'i' : 'n',
	     itr->get_local_choked() ? 'c' : 'u',
	     itr->get_local_interested() ? 'i' : 'n',
	     itr->get_choke_delayed() ? 'd' : ' ');
    x += 7;

    m_canvas->print(x, y, "%i/%i",
	     itr->get_outgoing_queue_size(),
	     itr->get_incoming_queue_size());
    x += 6;

    m_canvas->print(x, y, "%3i", (itr->get_chunks_done() * 100) / m_download->get_download().get_chunks_total());
    x += 6;

    if (itr->get_incoming_queue_size())
      m_canvas->print(x, y, "%i",
	       itr->get_incoming_index(0));

    x += 6;

    if (itr->get_snubbed())
      m_canvas->print(x, y, "*");

    ++y;
    ++itr;
  }
}

}
