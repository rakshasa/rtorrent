#ifndef RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H
#define RTORRENT_DISPLAY_WINDOW_DOWNLOAD_LIST_H

#include "window.h"

#include "core/download_list.h"
#include "core/view.h"

namespace display {

class WindowDownloadList : public Window {
public:
  typedef core::View::signal_void::iterator signal_void_itr;
  typedef std::pair<core::View::iterator, core::View::iterator> ViewRange;

  WindowDownloadList();
  ~WindowDownloadList();

  virtual void        redraw();

  void                set_view(core::View* l);

  int                 page_size(const std::string layout_name);
  int                 page_size();

private:
  core::View*         m_view{};

  std::pair<int, int> get_attr_color(core::View::iterator selected);
  signal_void_itr     m_changed_itr;
};

}

#endif
