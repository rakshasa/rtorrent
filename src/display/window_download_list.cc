#include "config.h"

#include "core/download.h"
#include "rak/algorithm.h"

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
  m_nextDraw = utils::Timer::cache().round_seconds() + 1000000;

  m_canvas->erase();

  if (m_list->empty())
    return;

  typedef std::pair<DList::iterator, DList::iterator> Range;

  Range range = rak::advance_bidirectional(m_list->begin(),
					   *m_focus != m_list->end() ? *m_focus : m_list->begin(),
					   m_list->end(),
					   m_canvas->get_height() / 3);

  // Make sure we properly fill out the last lines so it looks like
  // there are more torrents, yet don't hide it if we got the last one
  // in focus.
  if (range.second != m_list->end())
    ++range.second;

  int pos = 0;

  while (range.first != range.second) {
    torrent::Download& d = (*range.first)->get_download();

    m_canvas->print(0, pos++, "%c %s",
		    range.first == *m_focus ? '*' : ' ',
		    d.get_name().c_str());

    if ((*range.first)->is_open() && (*range.first)->is_done())
      m_canvas->print(0, pos++, "%c Torrent: Done %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		      range.first == *m_focus ? '*' : ' ',
		      (double)d.get_bytes_total() / (double)(1 << 20),
		      (double)d.get_rate_up() / 1024.0,
		      (double)d.get_rate_down() / 1024.0,
		      (double)d.get_bytes_up() / (double)(1 << 20));
    else
      m_canvas->print(0, pos++, "%c Torrent: %.1f / %.1f MiB Rate:%5.1f /%5.1f KiB Uploaded: %.1f MiB",
		      range.first == *m_focus ? '*' : ' ',
		      (double)d.get_bytes_done() / (double)(1 << 20),
		      (double)d.get_bytes_total() / (double)(1 << 20),
		      (double)d.get_rate_up() / 1024.0,
		      (double)d.get_rate_down() / 1024.0,
		      (double)d.get_bytes_up() / (double)(1 << 20));
    
    m_canvas->print(0, pos++, "%c %s",
		    range.first == *m_focus ? '*' : ' ',
		    print_download_status(*range.first).c_str());

    ++range.first;
  }    
}

}
