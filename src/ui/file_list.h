#ifndef RTORRENT_UI_FILE_LIST_H
#define RTORRENT_UI_FILE_LIST_H

#include "core/download.h"
#include "display/manager.h"
#include "input/bindings.h"

namespace display {
  class WindowFileList;
}

namespace ui {

class Control;

class FileList {
public:
  typedef display::WindowFileList    WFileList;
  typedef display::Manager::iterator MItr;

  FileList(Control* c, core::Download* d);

  void                activate(MItr mItr);
  void                disable();

  input::Bindings&    get_bindings() { return m_bindings; }

private:
  void                receive_next();
  void                receive_prev();

  void                receive_priority();

  core::Download*     m_download;
  WFileList*          m_window;
  
  Control*            m_control;
  input::Bindings     m_bindings;

  // Change to unsigned, please.
  unsigned int        m_focus;
};

}

#endif
