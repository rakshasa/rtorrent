#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOADS_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOADS_H

#include "downloads.h"
#include "window.h"

namespace display {

class WindowDownloads : public Window {
public:
  WindowDownloads(Downloads* d);

  virtual void redraw();

private:
  Downloads* m_downloads;
};

}

#endif
