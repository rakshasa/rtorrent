#include "config.h"

#include <torrent/exceptions.h>

#include "display/frame.h"
#include "display/window.h"

#include "element_base.h"

namespace ui {

void
ElementBase::mark_dirty() {
  if (is_active())
    m_frame->window()->mark_dirty();
}

}
