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

#include <fstream>
#include <stdexcept>
#include <string.h>
#include <rak/string_manip.h>
#include <torrent/throttle.h>
#include <torrent/torrent.h>
#include <torrent/download/resource_manager.h>
#include <torrent/utils/log.h>

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
#include "core/download_store.h"

#include "root.h"

namespace ui {

Root::Root() :
  m_control(NULL),
  m_downloadList(NULL),
  m_windowTitle(NULL),
  m_windowHttpQueue(NULL),
  m_windowInput(NULL),
  m_windowStatusbar(NULL),
  m_input_history_length(99),
  m_input_history_last_input(""),
  m_input_history_pointer_get(0) {

  // Initialise prefilled m_input_history and m_input_history_pointers objects.
  for (int type = ui::DownloadList::INPUT_LOAD_DEFAULT; type != ui::DownloadList::INPUT_EOI; type++) {
    m_input_history.insert( std::make_pair(type, InputHistoryCategory(m_input_history_length)) );
    m_input_history_pointers.insert( std::make_pair(type, 0) );
  }

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

  m_bindings[keys[ 0]]      = std::bind(&Root::adjust_up_throttle, this, (int) rpc::call_command_value("ui.throttle.global.step.small"));
  m_bindings[keys[ 1]]      = std::bind(&Root::adjust_up_throttle, this, (int) -rpc::call_command_value("ui.throttle.global.step.small"));
  m_bindings[keys[ 2]]      = std::bind(&Root::adjust_down_throttle, this, (int) rpc::call_command_value("ui.throttle.global.step.small"));
  m_bindings[keys[ 3]]      = std::bind(&Root::adjust_down_throttle, this, (int) -rpc::call_command_value("ui.throttle.global.step.small"));

  m_bindings[keys[ 4]]      = std::bind(&Root::adjust_up_throttle, this, (int) rpc::call_command_value("ui.throttle.global.step.medium"));
  m_bindings[keys[ 5]]      = std::bind(&Root::adjust_up_throttle, this, (int) -rpc::call_command_value("ui.throttle.global.step.medium"));
  m_bindings[keys[ 6]]      = std::bind(&Root::adjust_down_throttle, this, (int) rpc::call_command_value("ui.throttle.global.step.medium"));
  m_bindings[keys[ 7]]      = std::bind(&Root::adjust_down_throttle, this, (int) -rpc::call_command_value("ui.throttle.global.step.medium"));

  m_bindings[keys[ 8]]      = std::bind(&Root::adjust_up_throttle, this, (int) rpc::call_command_value("ui.throttle.global.step.large"));
  m_bindings[keys[ 9]]      = std::bind(&Root::adjust_up_throttle, this, (int) -rpc::call_command_value("ui.throttle.global.step.large"));
  m_bindings[keys[10]]      = std::bind(&Root::adjust_down_throttle, this, (int) rpc::call_command_value("ui.throttle.global.step.large"));
  m_bindings[keys[11]]      = std::bind(&Root::adjust_down_throttle, this, (int) -rpc::call_command_value("ui.throttle.global.step.large"));

  m_bindings['\x0C']        = std::bind(&display::Manager::force_redraw, m_control->display()); // ^L
  m_bindings['\x11']        = std::bind(&Control::receive_normal_shutdown, m_control); // ^Q
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
Root::enable_input(const std::string& title, input::TextInput* input, ui::DownloadList::Input type) {
  if (m_windowInput->input() != NULL)
    throw torrent::internal_error("Root::enable_input(...) m_windowInput->input() != NULL.");

  input->slot_dirty(std::bind(&WInput::mark_dirty, m_windowInput));

  m_windowStatusbar->set_active(false);

  m_windowInput->set_active(true);
  m_windowInput->set_input(input);
  m_windowInput->set_title(title);
  m_windowInput->set_focus(true);

  reset_input_history_attributes(type);

  input->bindings()['\x0C'] = std::bind(&display::Manager::force_redraw, m_control->display()); // ^L
  input->bindings()['\x11'] = std::bind(&Control::receive_normal_shutdown, m_control); // ^Q
  input->bindings()[KEY_UP] = std::bind(&Root::prev_in_input_history, this, type); // UP arrow
  input->bindings()['\x10'] = std::bind(&Root::prev_in_input_history, this, type); // ^P
  input->bindings()[KEY_DOWN] = std::bind(&Root::next_in_input_history, this, type); // DOWN arrow
  input->bindings()['\x0E'] = std::bind(&Root::next_in_input_history, this, type); // ^N

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

void
Root::add_to_input_history(ui::DownloadList::Input type, std::string item) {
  InputHistory::iterator itr = m_input_history.find(type);
  InputHistoryPointers::iterator pitr = m_input_history_pointers.find(type);
  int prev_item_pointer = (pitr->second - 1) < 0 ? (m_input_history_length - 1) : (pitr->second - 1);

  // Don't store item if it's empty or the same as the last one in the category.
  if (!item.empty() && item != itr->second.at(prev_item_pointer)) {
      itr->second.at(pitr->second) = rak::trim(item);
      m_input_history_pointers[type] = (pitr->second + 1) % m_input_history_length;
  }
}

void
Root::prev_in_input_history(ui::DownloadList::Input type) {
  if (m_windowInput->input() == NULL)
    throw torrent::internal_error("Root::prev_in_input_history() m_windowInput->input() == NULL.");

  InputHistory::iterator itr = m_input_history.find(type);
  InputHistoryPointers::const_iterator pitr = m_input_history_pointers.find(type);

  if (m_input_history_pointer_get == pitr->second)
    m_input_history_last_input = m_windowInput->input()->str();
  else
    itr->second.at(m_input_history_pointer_get) = m_windowInput->input()->str();

  std::string tmp_input = m_input_history_last_input;
  int prev_pointer_get = (m_input_history_pointer_get - 1) < 0 ? (m_input_history_length - 1) : (m_input_history_pointer_get - 1);

  if (prev_pointer_get != pitr->second && itr->second.at(prev_pointer_get) != "")
    m_input_history_pointer_get = prev_pointer_get;

  if (m_input_history_pointer_get != pitr->second)
    tmp_input = itr->second.at(m_input_history_pointer_get);

  m_windowInput->input()->str() = tmp_input;
  m_windowInput->input()->set_pos(tmp_input.length());
  m_windowInput->input()->mark_dirty();
}

void
Root::next_in_input_history(ui::DownloadList::Input type) {
  if (m_windowInput->input() == NULL)
    throw torrent::internal_error("Root::next_in_input_history() m_windowInput->input() == NULL.");

  InputHistory::iterator itr = m_input_history.find(type);
  InputHistoryPointers::const_iterator pitr = m_input_history_pointers.find(type);

  if (m_input_history_pointer_get == pitr->second)
    m_input_history_last_input = m_windowInput->input()->str();
  else
    itr->second.at(m_input_history_pointer_get) = m_windowInput->input()->str();

  std::string tmp_input = m_input_history_last_input;

  if (m_input_history_pointer_get != pitr->second) {
    m_input_history_pointer_get = (m_input_history_pointer_get + 1) % m_input_history_length;
    tmp_input = (m_input_history_pointer_get == pitr->second) ? m_input_history_last_input : itr->second.at(m_input_history_pointer_get);
  }

  m_windowInput->input()->str() = tmp_input;
  m_windowInput->input()->set_pos(tmp_input.length());
  m_windowInput->input()->mark_dirty();
}

void
Root::reset_input_history_attributes(ui::DownloadList::Input type) {
  InputHistoryPointers::const_iterator itr = m_input_history_pointers.find(type);

  // Clear last_input and set pointer_get to the same as pointer_insert.
  m_input_history_last_input = "";
  m_input_history_pointer_get = itr->second;
}

void
Root::set_input_history_size(int size) {
  if (size < 1)
    throw torrent::input_error("Invalid input history size.");

  for (InputHistory::iterator itr = m_input_history.begin(), last = m_input_history.end(); itr != last; itr++) {
    // Reserve the latest input history entries if new size is smaller than original.
    if (size < m_input_history_length) {
      int pointer_offset = m_input_history_length - size;
      InputHistoryPointers::iterator pitr = m_input_history_pointers.find(itr->first);
      InputHistoryCategory input_history_category_tmp = itr->second;

      for (int i=0; i != size; i++)
        itr->second.at(i) = input_history_category_tmp.at((pitr->second + pointer_offset + i) % m_input_history_length);

      m_input_history_pointers[pitr->first] = 0;
    }

    itr->second.resize(size);
  }

  m_input_history_length = size;
}

void
Root::load_input_history() {
  if (m_control == NULL || !m_control->core()->download_store()->is_enabled()) {
    lt_log_print(torrent::LOG_DEBUG, "ignoring input history file");
    return;
  }

  std::string history_filename = m_control->core()->download_store()->path() + "rtorrent.input_history";
  std::fstream history_file(history_filename.c_str(), std::ios::in);

  if (history_file.is_open()) {
    // Create a temp object of the content since size of history categories can be smaller than this.
    InputHistory input_history_tmp;

    for (int type = ui::DownloadList::INPUT_LOAD_DEFAULT; type != ui::DownloadList::INPUT_EOI; type++)
      input_history_tmp.insert( std::make_pair(type, InputHistoryCategory()) );

    std::string line;

    while (std::getline(history_file, line)) {
      if (!line.empty()) {
        int delim_pos = line.find("|");

        if (delim_pos != std::string::npos) {
          int type = std::atoi(line.substr(0, delim_pos).c_str());
          InputHistory::iterator itr = input_history_tmp.find(type);

          if (itr != input_history_tmp.end()) {
            std::string input_str = rak::trim(line.substr(delim_pos + 1));

            if (!input_str.empty())
              itr->second.push_back(input_str);
          }
        }
      }
    }

    if (history_file.bad()) {
      lt_log_print(torrent::LOG_DEBUG, "input history file corrupted, discarding (path:%s)", history_filename.c_str());
      return;
    } else {
      lt_log_print(torrent::LOG_DEBUG, "input history file read (path:%s)", history_filename.c_str());
    }

    for (InputHistory::const_iterator itr = input_history_tmp.begin(), last = input_history_tmp.end(); itr != last; itr++) {
      int input_history_tmp_category_length = itr->second.size();
      InputHistory::iterator hitr = m_input_history.find(itr->first);
      InputHistoryPointers::iterator pitr = m_input_history_pointers.find(itr->first);

      if (m_input_history_length < input_history_tmp_category_length) {
        int pointer_offset = input_history_tmp_category_length - m_input_history_length;

        for (int i=0; i != m_input_history_length; i++)
          hitr->second.at(i) = itr->second.at((pointer_offset + i) % input_history_tmp_category_length);

        pitr->second = 0;
      } else {
        hitr->second = itr->second;
        hitr->second.resize(m_input_history_length);

        pitr->second = input_history_tmp_category_length % m_input_history_length;
      }
    }
  } else {
    lt_log_print(torrent::LOG_DEBUG, "could not open input history file (path:%s)", history_filename.c_str());
  }
}

void
Root::save_input_history() {
  if (m_control == NULL || !m_control->core()->download_store()->is_enabled())
    return;

  std::string history_filename = m_control->core()->download_store()->path() + "rtorrent.input_history";
  std::string history_filename_tmp = history_filename + ".new";
  std::fstream history_file(history_filename_tmp.c_str(), std::ios::out | std::ios::trunc);

  if (!history_file.is_open()) {
    lt_log_print(torrent::LOG_DEBUG, "could not open input history file for writing (path:%s)", history_filename.c_str());
    return;
  }

  for (InputHistory::const_iterator itr = m_input_history.begin(), last = m_input_history.end(); itr != last; itr++) {
    InputHistoryPointers::const_iterator pitr = m_input_history_pointers.find(itr->first);

    for (int i=0; i != m_input_history_length; i++)
      if (!itr->second.at((pitr->second + i) % m_input_history_length).empty())
        history_file << itr->first << "|" + itr->second.at((pitr->second + i) % m_input_history_length) + "\n";
  }

  if (!history_file.good()) {
    lt_log_print(torrent::LOG_DEBUG, "input history file corrupted during writing, discarding (path:%s)", history_filename.c_str());
    return;
  } else {
    lt_log_print(torrent::LOG_DEBUG, "input history file written (path:%s)", history_filename.c_str());
  }

  history_file.close();

  std::rename(history_filename_tmp.c_str(), history_filename.c_str());
}

void
Root::clear_input_history() {
  for (int type = ui::DownloadList::INPUT_LOAD_DEFAULT; type != ui::DownloadList::INPUT_EOI; type++) {
    InputHistory::iterator itr = m_input_history.find(type);

    for (int i=0; i != m_input_history_length; i++)
      itr->second.at(i) = "";

    m_input_history_pointers[type] = 0;
  }
}

}
