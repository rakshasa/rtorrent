#ifndef RTORRENT_DISPLAY_FILE_LIST_H
#define RTORRENT_DISPLAY_FILE_LIST_H

#include <list>

#include "window.h"

namespace torrent {
  class Entry;
}

namespace core {
  class Download;
}

namespace display {

class WindowFileList : public Window {
public:
  WindowFileList(core::Download* d, unsigned int* focus);

  virtual void     redraw();

private:
  int              done_percentage(torrent::Entry& e);

  core::Download*  m_download;

  unsigned int*    m_focus;
};

}

#endif
