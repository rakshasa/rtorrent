#ifndef RTORRENT_UI_DOWNLOAD_LIST_H
#define RTORRENT_UI_DOWNLOAD_LIST_H

namespace display {
  class WindowTitle;
  class WindowDownloadList;
}

namespace ui {

class Control;
class Download;

class DownloadList {
public:
  typedef display::WindowDownloadList WList;
  typedef display::WindowTitle        WTitle;

  // We own 'window'.
  DownloadList(core::DownloadList* l, Control* c);
  ~DownloadList();

  WList&           get_window()   { return *m_window; }
  input::Bindings& get_bindings() { return *m_bindings; }

  void             activate();
  void             disable();

private:
  void             receive_next();
  void             receive_prev();

  void             receive_view_download();
  void             receive_exit_download();

  WList*           m_window;
  WTitle*          m_title;

  Download*        m_download;

  Control*         m_control;
  input::Bindings* m_bindings;
};

}

#endif
