// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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
#include <torrent/throttle.h>
#include <torrent/torrent.h>
#include <torrent/download/resource_manager.h>

#include "core/manager.h"
#include "display/frame.h"
#include "display/window_http_queue.h"
#include "display/window_title.h"
#include "display/window_input.h"
#include "display/window_statusbar.h"
#include "input/manager.h"
#include "input/text_input.h"
#include "rpc/parse_commands.h"

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
  m_windowStatusbar(NULL) {
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

const char*
Root::get_throttle_keys() {
  const std::string& keyLayout = rpc::call_command_string("keys.layout");

  if (strcasecmp(keyLayout.c_str(), "azerty") == 0)
    return "qwQWsxSXdcDC";
  else if (strcasecmp(keyLayout.c_str(), "qwertz") == 0)
    return "ayAYsxSXdcDC";
  else if (strcasecmp(keyLayout.c_str(), "dvorak") == 0)
    return "a;A:oqOQejEJ";
  else
    return "azAZsxSXdcDC";
}

void
Root::setup_keys() {
  m_control->input()->push_back(&m_bindings);

  const char* keys = get_throttle_keys();

  m_bindings[keys[ 0]]      = std::tr1::bind(&Root::adjust_up_throttle, this, 1);
  m_bindings[keys[ 1]]      = std::tr1::bind(&Root::adjust_up_throttle, this, -1);
  m_bindings[keys[ 2]]      = std::tr1::bind(&Root::adjust_down_throttle, this, 1);
  m_bindings[keys[ 3]]      = std::tr1::bind(&Root::adjust_down_throttle, this, -1);

  m_bindings[keys[ 4]]      = std::tr1::bind(&Root::adjust_up_throttle, this, 5);
  m_bindings[keys[ 5]]      = std::tr1::bind(&Root::adjust_up_throttle, this, -5);
  m_bindings[keys[ 6]]      = std::tr1::bind(&Root::adjust_down_throttle, this, 5);
  m_bindings[keys[ 7]]      = std::tr1::bind(&Root::adjust_down_throttle, this, -5);

  m_bindings[keys[ 8]]      = std::tr1::bind(&Root::adjust_up_throttle, this, 50);
  m_bindings[keys[ 9]]      = std::tr1::bind(&Root::adjust_up_throttle, this, -50);
  m_bindings[keys[10]]      = std::tr1::bind(&Root::adjust_down_throttle, this, 50);
  m_bindings[keys[11]]      = std::tr1::bind(&Root::adjust_down_throttle, this, -50);

  m_bindings['\x0C']        = std::tr1::bind(&display::Manager::force_redraw, m_control->display()); // ^L
  m_bindings['\x11']        = std::tr1::bind(&Control::receive_normal_shutdown, m_control); // ^Q
}

void
Root::set_down_throttle(unsigned int throttle) {
  if (m_windowStatusbar != NULL)
    m_windowStatusbar->mark_dirty();

  torrent::down_throttle_global()->set_max_rate(throttle * 1024);

  unsigned int div    = std::max<int>(rpc::call_command_value("throttle.max_downloads.div"), 0);
  unsigned int global = std::max<int>(rpc::call_command_value("throttle.max_downloads.global"), 0);

  if (throttle == 0 || div == 0) {
    torrent::resource_manager()->set_max_download_unchoked(global);
    return;
  }

  throttle /= div;

  unsigned int maxUnchoked;

  if (throttle <= 10)
    maxUnchoked = 1 + throttle / 1;
  else
    maxUnchoked = 10 + throttle / 5;

  if (global != 0)
    torrent::resource_manager()->set_max_download_unchoked(std::min(maxUnchoked, global));
  else
    torrent::resource_manager()->set_max_download_unchoked(maxUnchoked);
}

void
Root::set_up_throttle(unsigned int throttle) {
  if (m_windowStatusbar != NULL)
    m_windowStatusbar->mark_dirty();

  torrent::up_throttle_global()->set_max_rate(throttle * 1024);

  unsigned int div    = std::max<int>(rpc::call_command_value("throttle.max_uploads.div"), 0);
  unsigned int global = std::max<int>(rpc::call_command_value("throttle.max_uploads.global"), 0);

  if (throttle == 0 || div == 0) {
    torrent::resource_manager()->set_max_upload_unchoked(global);
    return;
  }

  throttle /= div;

  unsigned int maxUnchoked;

  if (throttle <= 10)
    maxUnchoked = 1 + throttle / 1;
  else
    maxUnchoked = 10 + throttle / 5;

  if (global != 0)
    torrent::resource_manager()->set_max_upload_unchoked(std::min(maxUnchoked, global));
  else
    torrent::resource_manager()->set_max_upload_unchoked(maxUnchoked);
}

void
Root::adjust_down_throttle(int throttle) {
  set_down_throttle(std::max<int>(torrent::down_throttle_global()->max_rate() / 1024 + throttle, 0));
}

void
Root::adjust_up_throttle(int throttle) {
  set_up_throttle(std::max<int>(torrent::up_throttle_global()->max_rate() / 1024 + throttle, 0));
}

void
Root::enable_input(const std::string& title, input::TextInput* input) {
  if (m_windowInput->input() != NULL)
    throw torrent::internal_error("Root::enable_input(...) m_windowInput->input() != NULL.");

  input->slot_dirty(std::tr1::bind(&WInput::mark_dirty, m_windowInput));

  m_windowStatusbar->set_active(false);

  m_windowInput->set_active(true);
  m_windowInput->set_input(input);
  m_windowInput->set_title(title);
  m_windowInput->set_focus(true);

  input->bindings()['\x0C'] = std::tr1::bind(&display::Manager::force_redraw, m_control->display()); // ^L
  input->bindings()['\x11'] = std::tr1::bind(&Control::receive_normal_shutdown, m_control); // ^Q

  control->input()->set_text_input(input);
  control->display()->adjust_layout();
}

void
Root::disable_input() {
  if (m_windowInput->input() == NULL)
    throw torrent::internal_error("Root::disable_input() m_windowInput->input() == NULL.");

  m_windowInput->input()->slot_dirty(ElementBase::slot_type());

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
