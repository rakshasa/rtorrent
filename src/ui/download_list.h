#ifndef RTORRENT_UI_DOWNLOAD_LIST_H
#define RTORRENT_UI_DOWNLOAD_LIST_H

#include "element_base.h"
#include "globals.h"
#include "display/manager.h"

class Control;

namespace core {
  class Download;
  class View;
}

namespace input {
  class Bindings;
}

namespace display {
  class Frame;
  class WindowDownloadList;
  class WindowHttpQueue;
  class WindowInput;
  class WindowLog;
  class WindowLogComplete;
}

namespace ui {

class Download;

class DownloadList : public ElementBase {
public:
  typedef display::WindowDownloadList              WList;
  typedef display::WindowLog                       WLog;
  typedef display::WindowLogComplete               WLogComplete;

  typedef std::function<void (const std::string&)> slot_string;

  typedef enum {
    DISPLAY_DOWNLOAD,
    DISPLAY_DOWNLOAD_LIST,
    DISPLAY_LOG,
    DISPLAY_STRING_LIST,
    DISPLAY_MAX_SIZE
  } Display;

  typedef enum {
    INPUT_NONE,
    INPUT_LOAD_DEFAULT,
    INPUT_LOAD_MODIFIED,
    INPUT_CHANGE_DIRECTORY,
    INPUT_COMMAND,
    INPUT_FILTER,
    INPUT_EOI
  } Input;

  DownloadList();
  ~DownloadList();

  void                activate(display::Frame* frame, bool focus = true);
  void                disable();

  void                activate_display(Display d);

  core::View*         current_view();
  void                set_current_view(const std::string& name);

  void                slot_open_uri(slot_string s) { m_slot_open_uri = s; }

  void                unfocus_download(core::Download* d);

private:
  DownloadList(const DownloadList&);
  void operator = (const DownloadList&);

  void                receive_view_input(Input type);
  void                receive_exit_input(Input type);

  void                setup_keys();
  void                setup_input();

  Display             m_state{DISPLAY_MAX_SIZE};

  ElementBase*        m_uiArray[DISPLAY_MAX_SIZE];
  WLog*               m_windowLog;

  slot_string         m_slot_open_uri;
};

}

#endif
