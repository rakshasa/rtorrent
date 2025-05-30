#ifndef RTORRENT_UI_ELEMENT_DOWNLOAD_LIST_H
#define RTORRENT_UI_ELEMENT_DOWNLOAD_LIST_H

#include "display/window_download_list.h"

#include "element_base.h"

class Control;

namespace core {
class View;
}

namespace ui {

class ElementDownloadList : public ElementBase {
public:
  typedef display::WindowDownloadList WDownloadList;

  ElementDownloadList();

  void        activate(display::Frame* frame, bool focus = true);
  void        disable();

  core::View* view() { return m_view; }
  void        set_view(core::View* l);

  void        receive_command(const char* cmd);

  void        receive_next();
  void        receive_prev();

  int         page_size();
  void        receive_pagenext();
  void        receive_pageprev();

  void        receive_home();
  void        receive_end();

  void        receive_stop_download();
  void        receive_close_download();

  void        receive_next_priority();
  void        receive_prev_priority();

  void        receive_cycle_throttle();

  void        receive_change_view(const std::string& name);

  void        toggle_layout();

private:
  WDownloadList* m_window{};
  core::View*    m_view{};
};

} // namespace ui

#endif
