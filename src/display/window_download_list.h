#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H

#include "window.h"

namespace engine {
  class DownloadList;
}

namespace display {

class WindowDownloadList : public Window {
public:
  WindowDownloadList(engine::DownloadList* d);

  virtual void redraw();

private:
  engine::DownloadList* m_downloads;
};

}

#endif
