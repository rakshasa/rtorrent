#include "config.h"

#include "canvas.h"
#include "window_download_list.h"

#include "engine/download_list.h"

namespace display {

WindowDownloadList::WindowDownloadList(engine::DownloadList* d) :
  Window(new Canvas, true),
  m_downloads(d) {
}

void
WindowDownloadList::redraw() {
  m_canvas->erase();
  m_canvas->print_border(' ', ' ', '-', '-', ' ', ' ', ' ', ' ');

  int pos = 1;

  for (engine::DownloadList::iterator itr = m_downloads->begin(); itr != m_downloads->end(); ++itr, ++pos)
    m_canvas->print(1, pos, "Download: %s Open: %c Tracker: %c Active: %c",
		    itr->get_download().get_name().c_str(),
		    itr->get_download().is_open() ? 'Y' : 'N',
		    itr->get_download().is_tracker_busy() ? 'Y' : 'N',
		    itr->get_download().is_active() ? 'Y' : 'N');
}

}
