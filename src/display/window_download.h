#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOAD_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOAD_H

#include "window.h"
#include "core/download_list.h"

namespace display {

class WindowDownload : public Window {
public:
  typedef core::DownloadList::iterator DPtr;

  WindowDownload(DPtr d);

  DPtr           get_download() { return m_download; }

  virtual void   redraw();

private:
  DPtr           m_download;
};

}

#endif
