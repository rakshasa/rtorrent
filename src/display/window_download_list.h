#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H

#include "window.h"
#include "core/download_list.h"

namespace display {

class WindowDownloadList : public Window {
public:
  typedef core::DownloadList List;

  WindowDownloadList(List* d);

  List::iterator get_focus()                                   { return m_focus; }
  void           set_focus(core::DownloadList::iterator itr) { m_focus = itr; }

  virtual void   redraw();

private:
  List*          m_downloads;
  List::iterator m_focus;
};

}

#endif
