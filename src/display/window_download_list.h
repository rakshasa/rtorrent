#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H

#include "window.h"
#include "core/download_list.h"

namespace display {

class WindowDownloadList : public Window {
public:
  typedef core::DownloadList DList;

  WindowDownloadList(DList* l, DList::iterator* focus);

  virtual void     redraw();

private:
  DList*           m_list;
  DList::iterator* m_focus;
};

}

#endif
