#include "config.h"

#include "canvas.h"
#include "window_download.h"

namespace display {

WindowDownload::WindowDownload(DPtr d) :
  Window(new Canvas, true),
  m_download(d) {
}

void
WindowDownload::redraw() {
  m_canvas->erase();

  m_canvas->print(0, 0, "Download: %s", m_download->get_download().get_name().c_str());
}

}
