#ifndef RTORRENT_UI_ELEMENT_FILE_LIST_H
#define RTORRENT_UI_ELEMENT_FILE_LIST_H

#include <torrent/common.h>
#include <torrent/data/file_list_iterator.h>

#include "core/download.h"

#include "element_base.h"

namespace display {
  class WindowFileList;
}

namespace ui {

class ElementText;

class ElementFileList : public ElementBase {
public:
  typedef display::WindowFileList   WFileList;
  typedef torrent::FileListIterator iterator;

  typedef enum {
    DISPLAY_LIST,
    DISPLAY_INFO,
    DISPLAY_MAX_SIZE
  } Display;

  ElementFileList(core::Download* d);

  void                activate(display::Frame* frame, bool focus = true);
  void                disable();

  void                activate_display(Display display);

  bool                is_collapsed() const  { return m_collapsed; }
  void                set_collapsed(bool s) { m_collapsed = s; }

  iterator            selected() const      { return m_selected; }
  core::Download*     download() const      { return m_download; }

private:
  void                receive_next();
  void                receive_prev();
  void                receive_pagenext();
  void                receive_pageprev();

  void                receive_select();

  void                receive_priority();
  void                receive_change_all();
  void                receive_collapse();

  void                update_itr();

  core::Download*     m_download;

  Display             m_state{DISPLAY_MAX_SIZE};
  WFileList*          m_window{};
  ElementText*        m_elementInfo{};

  // Change to unsigned, please.
  iterator            m_selected;
  bool                m_collapsed{};
};

}

#endif
