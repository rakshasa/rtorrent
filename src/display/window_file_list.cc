#include "config.h"

#include <stdexcept>

#include "core/download.h"
#include "utils/algorithm.h"

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

  int pos = 0;

  m_canvas->print( 2, pos, "File");
  m_canvas->print(55, pos, "Size");
  m_canvas->print(62, pos, "Pri");
  m_canvas->print(67, pos, "Cmpl");

  ++pos;

  if (m_download->get_download().get_entry_size() == 0)
    return;

  if (*m_focus >= m_download->get_download().get_entry_size())
    throw std::logic_error("WindowFileList::redraw() called on an object with a bad focus value");

  typedef std::pair<unsigned int, unsigned int> Range;

  Range range = utils::advance_bidirectional<unsigned int>(0, *m_focus, m_download->get_download().get_entry_size(),
							   m_canvas->get_height());

  while (range.first != range.second) {
    torrent::Entry e = m_download->get_download().get_entry(range.first);

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

    m_canvas->print(0, pos, "%c %s  %5.1f   %s   %3d",
		    range.first == *m_focus ? '*' : ' ',
		    path.c_str(),
		    (double)e.get_size() / (double)(1 << 20),
		    priority.c_str(),
		    done_percentage(e));

    ++range.first;
    ++pos;
  }

}

int
WindowFileList::done_percentage(torrent::Entry& e) {
  int chunks = e.get_chunk_end() - e.get_chunk_begin();

  return chunks ? (e.get_completed() * 100) / chunks : 100;
}

}
