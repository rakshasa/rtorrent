#ifndef RTORRENT_UI_ELEMENT_LOG_COMPLETE_H
#define RTORRENT_UI_ELEMENT_LOG_COMPLETE_H

#include <torrent/utils/log_buffer.h>

#include "element_base.h"

class Control;

namespace display {
  class WindowLogComplete;
}

namespace ui {

class ElementLogComplete : public ElementBase {
public:
  typedef display::WindowLogComplete    WLogComplete;

  ElementLogComplete(torrent::log_buffer* l);

  void                activate(display::Frame* frame, bool focus = true);
  void                disable();

  display::Window*    window();

private:
  void                received_update();

  WLogComplete*       m_window{};

  torrent::log_buffer* m_log;
};

}

#endif
