#include "config.h"

#include "core/download.h"

#include "window_file_list.h"

namespace display {

WindowFileList::WindowFileList(core::Download* d, unsigned int* focus) :
  Window(new Canvas, true),
  m_download(d),
  m_focus(focus) {
}

void
WindowFileList::redraw() {
  m_nextDraw = utils::Timer::cache().round_seconds() + 10 * 1000000;
  m_canvas->erase();

  int y1 = 0;
  int y2 = m_canvas->get_height();

  m_canvas->print( 2, y1, "File");
  m_canvas->print(55, y1, "Size");
  m_canvas->print(62, y1, "Pri");
  m_canvas->print(67, y1, "Cmpl");

  ++y1;

  unsigned int files = m_download->get_download().get_entry_size();
  unsigned int index = std::min<unsigned>(std::max((int)*m_focus - (y2 - y1) / 2, 0), (int)files - (y2 - y1));

  while (index < files && y1 < y2) {
    torrent::Entry e = m_download->get_download().get_entry(index);

    std::string path = e.get_path();

    if (path.length() <= 50)
      path = path + std::string(50 - path.length(), ' ');
    else
      path = path.substr(0, 50);

    std::string priority;

    switch (e.get_priority()) {
    case torrent::Entry::STOPPED:
      priority = "off";
      break;

    case torrent::Entry::NORMAL:
      priority = "   ";
      break;

    case torrent::Entry::HIGH:
      priority = "hig";
      break;

    default:
      priority = "BUG";
      break;
    };

    m_canvas->print(0, y1, "%c %s  %5.1f   %s   %3d",
		    index == *m_focus ? '*' : ' ',
		    path.c_str(),
		    (double)e.get_size() / (double)(1 << 20),
		    priority.c_str(),
		    done_percentage(e));

    ++index;
    ++y1;
  }

}

int
WindowFileList::done_percentage(torrent::Entry& e) {
  int chunks = e.get_chunk_end() - e.get_chunk_begin();

  return chunks ? (e.get_completed() * 100) / chunks : 100;
}

}
