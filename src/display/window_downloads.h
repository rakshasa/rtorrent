#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOADS_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOADS_H

#include "window.h"
#include "engine/downloads.h"

namespace display {

class WindowDownloads : public Window {
public:
  WindowDownloads(engine::Downloads* d);

  virtual void redraw();

private:
  engine::Downloads* m_downloads;
};

}

#endif
