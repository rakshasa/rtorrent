#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H

#include "window.h"
#include "core/download_list.h"

namespace display {

class WindowDownloadList : public Window {
public:
  typedef core::DownloadList DList;

  WindowDownloadList(DList* d);

  DList&          get_list()                     { return *m_downloads; }

  DList::iterator get_focus()                    { return m_focus; }
  void            set_focus(DList::iterator itr) { m_focus = itr; }

  virtual void    redraw();

private:
  DList*          m_downloads;
  DList::iterator m_focus;
};

}

#endif
