#ifndef RTORRENT_UI_DOWNLOAD_LIST_H
#define RTORRENT_UI_DOWNLOAD_LIST_H

#include <sigc++/slot.h>

#include "core/download_list.h"
#include "utils/task.h"

namespace input {
  class Bindings;
}

namespace display {
  class WindowDownloadList;
  class WindowHttpQueue;
  class WindowInput;
  class WindowLog;
  class WindowStatusbar;
  class WindowTitle;
}

namespace ui {

class Control;
class Download;

class DownloadList {
public:
  typedef display::WindowDownloadList           WList;
  typedef display::WindowHttpQueue              WHttp;
  typedef display::WindowInput                  WInput;
  typedef display::WindowLog                    WLog;
  typedef display::WindowStatusbar              WStatus;
  typedef display::WindowTitle                  WTitle;

  typedef core::DownloadList                    DList;

  typedef sigc::slot1<void, const std::string&> SlotOpenUri;

  // We own 'window'.
  DownloadList(Control* c);
  ~DownloadList();

  WList&           get_window()                 { return *m_window; }
  input::Bindings& get_bindings()               { return *m_bindings; }

  void             activate();
  void             disable();

  void             slot_open_uri(SlotOpenUri s) { m_slotOpenUri = s; }

private:
  DownloadList(const DownloadList&);
  void operator = (const DownloadList&);

  void             receive_next();
  void             receive_prev();

  void             receive_throttle(int t);

  void             receive_start_download();
  void             receive_stop_download();

  void             receive_view_download();
  void             receive_exit_download();

  void             receive_view_input();
  void             receive_exit_input();

  void             task_update();

  void             bind_keys(input::Bindings* b);

  void             mark_dirty();

  WList*           m_window;
  WTitle*          m_title;
  WStatus*         m_status;
  WLog*            m_windowLog;
  WInput*          m_textInput;
  WHttp*           m_windowHttpQueue;

  utils::Task      m_taskUpdate;

  Download*        m_download;

  DList*           m_list;
  DList::iterator  m_focus;

  Control*         m_control;
  input::Bindings* m_bindings;

  SlotOpenUri      m_slotOpenUri;
};

}

#endif
