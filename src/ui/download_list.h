#ifndef RTORRENT_UI_DOWNLOAD_LIST_H
#define RTORRENT_UI_DOWNLOAD_LIST_H

namespace display {
  class WindowTitle;
  class WindowDownloadList;
  class WindowStatusbar;
}

namespace ui {

class Control;
class Download;

class DownloadList {
public:
  typedef display::WindowDownloadList WList;
  typedef display::WindowTitle        WTitle;
  typedef display::WindowStatusbar    WStatus;
  typedef core::DownloadList          DList;

  // We own 'window'.
  DownloadList(DList* l, Control* c);
  ~DownloadList();

  WList&           get_window()   { return *m_window; }
  input::Bindings& get_bindings() { return *m_bindings; }

  void             activate();
  void             disable();

private:
  DownloadList(const DownloadList&);
  void operator = (const DownloadList&);

  void             receive_next();
  void             receive_prev();

  void             receive_view_download();
  void             receive_exit_download();

  void             receive_throttle(int t);

  void             bind_keys(input::Bindings* b);

  void             mark_dirty();

  WList*           m_window;
  WTitle*          m_title;
  WStatus*         m_status;

  Download*        m_download;

  DList*           m_list;
  DList::iterator  m_focus;

  Control*         m_control;
  input::Bindings* m_bindings;
};

}

#endif
