#include "config.h"

#include "core/download.h"

#include "canvas.h"
#include "utils.h"
#include "window_download_list.h"

namespace display {

WindowDownloadList::WindowDownloadList(DList* l, DList::iterator* f) :
  Window(new Canvas, true),
  m_list(l),
  m_focus(f) {
}

void
WindowDownloadList::redraw() {
  if (utils::Timer::cache() - m_lastDraw < 1000000)
    return;

  m_lastDraw = utils::Timer::cache();

  m_canvas->erase();

  if (m_list->empty())
    return;

  DList::iterator itr, last;
  
  int pos = 0, y = 0;

  if (*m_focus != m_list->end()) {
    int i = 0;
    itr = last = *m_focus;
    
    while (i < m_canvas->get_height() - y - 3 && !(itr == m_list->begin() && last == m_list->end())) {
      if (last != m_list->end()) {
	++last;
	i += 3;
      }

      if (itr != m_list->begin() && i < m_canvas->get_height() - y - 3) {
	--itr;
	i += 3;
      }
    }

  } else {
    itr = m_list->begin();
    last = m_list->end();
  }    

  while (itr != m_list->end() && y < m_canvas->get_height()) {
    m_canvas->print(0, pos++, "%c %s",
		    itr == *m_focus ? '*' : ' ',
		    (*itr)->get_download().get_name().c_str());

    if ((*itr)->get_download().get_chunks_done() != (*itr)->get_download().get_chunks_total() || !(*itr)->get_download().is_open())
      m_canvas->print(0, pos++, "%c Torrent: %.1f / %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		      itr == *m_focus ? '*' : ' ',
		      (double)(*itr)->get_download().get_bytes_done() / (double)(1 << 20),
		      (double)(*itr)->get_download().get_bytes_total() / (double)(1 << 20),
		      (double)(*itr)->get_download().get_rate_up() / 1024.0,
		      (double)(*itr)->get_download().get_rate_down() / 1024.0,
		      (double)(*itr)->get_download().get_bytes_up() / (double)(1 << 20));
 
    else
      m_canvas->print(0, pos++, "%c Torrent: Done %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		      itr == *m_focus ? '*' : ' ',
		      (double)(*itr)->get_download().get_bytes_total() / (double)(1 << 20),
		      (double)(*itr)->get_download().get_rate_up() / 1024.0,
		      (double)(*itr)->get_download().get_rate_down() / 1024.0,
		      (double)(*itr)->get_download().get_bytes_up() / (double)(1 << 20));
    
    m_canvas->print(0, pos++, "%c %s", itr == *m_focus ? '*' : ' ', print_download_status(*itr).c_str());

    ++itr;
  }    
}

}
