#include "config.h"

#include "canvas.h"
#include "window_download_list.h"

namespace display {

WindowDownloadList::WindowDownloadList(DList* l, DList::iterator* f) :
  Window(new Canvas, true),
  m_list(l),
  m_focus(f) {
}

void
WindowDownloadList::redraw() {
  if (Timer::cache() - m_lastDraw < 1000000)
    return;

  m_lastDraw = Timer::cache();

  m_canvas->erase();

  int pos = 0;

  // Remember to check for end of screen too.

  for (DList::iterator itr = m_list->begin(); itr != m_list->end(); ++itr) {
    m_canvas->print(0, pos++, "%c %s",
		    itr == *m_focus ? '*' : ' ',
		    itr->get_download().get_name().c_str());

    if (itr->get_download().get_chunks_done() != itr->get_download().get_chunks_total() || !itr->get_download().is_open())
      m_canvas->print(0, pos++, "%c Torrent: %.1f / %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		      itr == *m_focus ? '*' : ' ',
		      (double)itr->get_download().get_bytes_done() / (double)(1 << 20),
		      (double)itr->get_download().get_bytes_total() / (double)(1 << 20),
		      (double)itr->get_download().get_rate_up() / 1024.0,
		      (double)itr->get_download().get_rate_down() / 1024.0,
		      (double)itr->get_download().get_bytes_up() / (double)(1 << 20));
 
    else
      m_canvas->print(0, pos++, "%c Torrent: Done %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		      itr == *m_focus ? '*' : ' ',
		      (double)itr->get_download().get_bytes_total() / (double)(1 << 20),
		      (double)itr->get_download().get_rate_up() / 1024.0,
		      (double)itr->get_download().get_rate_down() / 1024.0,
		      (double)itr->get_download().get_bytes_up() / (double)(1 << 20));
    
    m_canvas->print(0, pos++, "%c Tracker: [%c:%i] %s",
		    itr == *m_focus ? '*' : ' ',
		    itr->get_download().is_tracker_busy() ? 'C' : ' ',
		    (int)(itr->get_download().get_tracker_timeout() / 1000000),
		    "");
  }    
}

}
