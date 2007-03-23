// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <stdexcept>
#include <string.h>
#include <sigc++/bind.h>
#include <torrent/torrent.h>

#include "core/manager.h"
#include "display/frame.h"
#include "display/window_http_queue.h"
#include "display/window_title.h"
#include "display/window_input.h"
#include "display/window_statusbar.h"
#include "input/manager.h"
#include "input/text_input.h"
#include "utils/variable_map.h"

#include "control.h"
#include "download_list.h"

#include "root.h"

namespace ui {

Root::Root() :
  m_control(NULL),
  m_downloadList(NULL),
  m_windowTitle(NULL),
  m_windowHttpQueue(NULL),
  m_windowInput(NULL),
  m_windowStatusbar(NULL),

  m_maxUploadsGlobal(0),
  m_maxDownloadsGlobal(0) {
}

void
Root::init(Control* c) {
  if (m_control != NULL)
    throw std::logic_error("Root::init() called twice on the same object");

  m_control = c;

  m_windowTitle     = new WTitle();
  m_windowHttpQueue = new WHttpQueue(control->core()->http_queue());
  m_windowInput     = new WInput();
  m_windowStatusbar = new WStatusbar();

  m_downloadList    = new DownloadList();

  display::Frame* rootFrame = m_control->display()->root_frame();

  rootFrame->initialize_row(5);
  rootFrame->frame(0)->initialize_window(m_windowTitle);
  rootFrame->frame(2)->initialize_window(m_windowHttpQueue);
  rootFrame->frame(3)->initialize_window(m_windowInput);
  rootFrame->frame(4)->initialize_window(m_windowStatusbar);

  m_windowTitle->set_active(true);
  m_windowStatusbar->set_active(true);
  m_windowStatusbar->set_bottom(true);

  setup_keys();

  m_downloadList->activate(rootFrame->frame(1));
}

void
Root::cleanup() {
  if (m_control == NULL)
    throw std::logic_error("Root::cleanup() called twice on the same object");

  if (m_downloadList->is_active())
    m_downloadList->disable();

  m_control->display()->root_frame()->clear();

  delete m_downloadList;

  delete m_windowTitle;
  delete m_windowHttpQueue;
  delete m_windowInput;
  delete m_windowStatusbar;

  m_control->input()->erase(&m_bindings);
  m_control = NULL;
}

void
Root::setup_keys() {
  m_control->input()->push_back(&m_bindings);

  if (strcasecmp(control->variable()->get_string("key_layout").c_str(), "azerty") == 0) {
    m_bindings['q']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 1);
    m_bindings['w']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -1);
    m_bindings['Q']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 1);
    m_bindings['W']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -1);

  } else if (strcasecmp(control->variable()->get_string("key_layout").c_str(), "qwertz") == 0) {
    m_bindings['a']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 1);
    m_bindings['y']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -1);
    m_bindings['A']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 1);
    m_bindings['Y']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -1);

  } else if (strcasecmp(control->variable()->get_string("key_layout").c_str(), "dvorak") == 0) {
    m_bindings['a']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 1);
    m_bindings[';']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -1);
    m_bindings['A']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 1);
    m_bindings[':']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -1);

  } else {
    m_bindings['a']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 1);
    m_bindings['z']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -1);
    m_bindings['A']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 1);
    m_bindings['Z']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -1);
  }

  if (strcasecmp(control->variable()->get_string("key_layout").c_str(), "dvorak") != 0) {
    m_bindings['s']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 5);
    m_bindings['x']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -5);
    m_bindings['S']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 5);
    m_bindings['X']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -5);

    m_bindings['d']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 50);
    m_bindings['c']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -50);
    m_bindings['D']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 50);
    m_bindings['C']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -50);

  } else {
    m_bindings['o']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 5);
    m_bindings['q']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -5);
    m_bindings['O']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 5);
    m_bindings['Q']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -5);

    m_bindings['e']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), 50);
    m_bindings['j']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_up_throttle), -50);
    m_bindings['E']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), 50);
    m_bindings['J']           = sigc::bind(sigc::mem_fun(*this, &Root::adjust_down_throttle), -50);
  }

  m_bindings['\x0C']        = sigc::mem_fun(m_control->display(), &display::Manager::force_redraw); // ^L
  m_bindings['\x11']        = sigc::mem_fun(m_control, &Control::receive_normal_shutdown); // ^Q
}

void
Root::set_down_throttle(unsigned int throttle) {
  if (m_windowStatusbar != NULL)
    m_windowStatusbar->mark_dirty();

  torrent::set_down_throttle(throttle * 1024);

  int64_t div = control->variable()->get_value("max_downloads_div");

  if (throttle == 0 || div <= 0) {
    torrent::set_max_download_unchoked(m_maxDownloadsGlobal);
    return;
  }

  throttle /= control->variable()->get_value("max_downloads_div");

  unsigned int maxUnchoked;

  if (throttle <= 10)
    maxUnchoked = 1 + throttle / 1;
  else
    maxUnchoked = 10 + throttle / 5;

  if (m_maxDownloadsGlobal != 0)
    torrent::set_max_download_unchoked(std::min(maxUnchoked, m_maxDownloadsGlobal));
  else
    torrent::set_max_download_unchoked(maxUnchoked);
}

void
Root::set_up_throttle(unsigned int throttle) {
  if (m_windowStatusbar != NULL)
    m_windowStatusbar->mark_dirty();

  torrent::set_up_throttle(throttle * 1024);

  int64_t div = control->variable()->get_value("max_uploads_div");

  if (throttle == 0 || div <= 0) {
    torrent::set_max_unchoked(m_maxUploadsGlobal);
    return;
  }

  throttle /= control->variable()->get_value("max_uploads_div");

  unsigned int maxUnchoked;

  if (throttle <= 10)
    maxUnchoked = 1 + throttle / 1;
  else
    maxUnchoked = 10 + throttle / 5;

  if (m_maxUploadsGlobal != 0)
    torrent::set_max_unchoked(std::min(maxUnchoked, m_maxUploadsGlobal));
  else
    torrent::set_max_unchoked(maxUnchoked);
}

void
Root::adjust_down_throttle(int throttle) {
  set_down_throttle(std::max<int>(torrent::down_throttle() / 1024 + throttle, 0));
}

void
Root::adjust_up_throttle(int throttle) {
  set_up_throttle(std::max<int>(torrent::up_throttle() / 1024 + throttle, 0));
}

void
Root::set_max_uploads_global(int64_t slots) {
  if (slots < 0)
    throw torrent::input_error("Out of range.");

  m_maxUploadsGlobal = slots;

  set_up_throttle(torrent::up_throttle() / 1024);
}

void
Root::set_max_downloads_global(int64_t slots) {
  if (slots < 0)
    throw torrent::input_error("Out of range.");

  m_maxDownloadsGlobal = slots;

  set_down_throttle(torrent::down_throttle() / 1024);
}

void
Root::enable_input(const std::string& title, input::TextInput* input) {
  if (m_windowInput->input() != NULL)
    throw torrent::internal_error("Root::enable_input(...) m_windowInput->input() != NULL.");

  input->slot_dirty(sigc::mem_fun(m_windowInput, &WInput::mark_dirty));

  m_windowStatusbar->set_active(false);

  m_windowInput->set_active(true);
  m_windowInput->set_input(input);
  m_windowInput->set_title(title);
  m_windowInput->set_focus(true);

  input->bindings()['\x0C'] = sigc::mem_fun(m_control->display(), &display::Manager::force_redraw); // ^L
  input->bindings()['\x11'] = sigc::mem_fun(m_control, &Control::receive_normal_shutdown); // ^Q

  control->input()->set_text_input(input);
  control->display()->adjust_layout();
}

void
Root::disable_input() {
  if (m_windowInput->input() == NULL)
    throw torrent::internal_error("Root::disable_input() m_windowInput->input() == NULL.");

  m_windowInput->input()->slot_dirty(sigc::slot0<void>());

  m_windowStatusbar->set_active(true);

  m_windowInput->set_active(false);
  m_windowInput->set_focus(false);
  m_windowInput->set_input(NULL);

  control->input()->set_text_input(NULL);
  control->display()->adjust_layout();
}

input::TextInput*
Root::current_input() {
  return m_windowInput->input();
}

}
