#include "config.h"

#include "canvas.h"
#include "window_input.h"

#include "input/text_input.h"

namespace display {

WindowInput::WindowInput(input::TextInput* input) :
  Window(new Canvas, false, 1),
  m_input(input),
  m_focus(false) {
}

void
WindowInput::redraw() {
  m_nextDraw = utils::Timer::max();
  m_canvas->erase();

  m_canvas->print(0, 0, "> %s", m_input->c_str());

  if (m_focus)
    m_canvas->set_attr(m_input->get_pos() + 2, 0, 1, A_REVERSE, COLOR_PAIR(0));
}

}
