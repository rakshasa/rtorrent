#include "config.h"

#include <sigc++/bind.h>

#include "control.h"
#include "download_list.h"

#include "root.h"

namespace ui {

void
Root::init() {
  setup_keys();

  m_downloadList = new DownloadList(m_control);

  m_downloadList->activate();
  m_downloadList->slot_open_uri(sigc::mem_fun(m_control->get_core(), &core::Manager::insert));
}

void
Root::cleanup() {
  delete m_downloadList;

  m_control->get_input().erase(&m_bindings);
}

void
Root::setup_keys() {
  m_control->get_input().push_back(&m_bindings);

  m_bindings[KEY_RESIZE] = sigc::mem_fun(m_control->get_display(), &display::Manager::adjust_layout);
  m_bindings['\x11']     = sigc::bind(sigc::mem_fun(*this, &Root::set_shutdown_received), true);
}

}
