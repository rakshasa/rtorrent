#ifndef RTORRENT_WINDOW_DOWNLOAD_STATUSBAR_H
#define RTORRENT_WINDOW_DOWNLOAD_STATUSBAR_H

#include "window.h"

namespace core {
  class Download;
}

namespace display {

class WindowDownloadStatusbar : public Window {
public:
  WindowDownloadStatusbar(core::Download* d);

  virtual void    redraw();

private:
  core::Download* m_download;
};

}

#endif
