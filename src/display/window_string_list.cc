#include "config.h"

#include "canvas.h"
#include "utils.h"
#include "window_string_list.h"

namespace display {

WindowStringList::WindowStringList() :
  Window(new Canvas, 0, 0, 0, extent_full, extent_full) {
}

WindowStringList::~WindowStringList() = default;

void
WindowStringList::redraw() {
  m_canvas->erase();

  size_t ypos = 0;
  size_t xpos = 1;
  size_t width = 0;

  iterator itr = m_first;

  while (itr != m_last) {

    if (ypos == (size_t)m_canvas->height()) {
      ypos = 0;
      xpos += width + 2;

      if (xpos + 20 >= (size_t)m_canvas->width())
        break;

      width = 0;
    }

    width = std::max(itr->size(), width);

    if (xpos + itr->size() <= (size_t)m_canvas->width())
      m_canvas->print(xpos, ypos++, "%s", itr->c_str());
    else
      m_canvas->print(xpos, ypos++, "%s", itr->substr(0, m_canvas->width() - xpos).c_str());

    ++itr;
  }

  m_drawEnd = itr;
}

}
