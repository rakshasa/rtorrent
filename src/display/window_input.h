#ifndef RTORRENT_DISPLAY_WINDOW_INPUT_H
#define RTORRENT_DISPLAY_WINDOW_INPUT_H

#include "window.h"

namespace input {
  class TextInput;
}

namespace display {

class WindowInput : public Window {
public:
  WindowInput(input::TextInput* input);

  input::TextInput* get_input()       { return m_input; }

  bool              get_focus()       { return m_focus; }
  void              set_focus(bool f) { mark_dirty(); m_focus = f; }

  virtual void      redraw();

private:
  input::TextInput* m_input;
  bool              m_focus;
};

}

#endif
