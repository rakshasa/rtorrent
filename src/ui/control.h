#ifndef RTORRENT_UI_CONTROL_H
#define RTORRENT_UI_CONTROL_H

#include "display/manager.h"
#include "input/manager.h"

namespace ui {

class Control {
public:
  
  display::Manager& get_display() { return m_display; }
  input::Manager&   get_input()   { return m_input; }

private:
  display::Manager  m_display;
  input::Manager    m_input;
};

}

#endif
