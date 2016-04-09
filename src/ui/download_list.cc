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

#include <rak/functional.h>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>
#include <torrent/utils/log.h>

#include "core/download.h"
#include "core/download_list.h"
#include "core/manager.h"
#include "core/view.h"
#include "core/view_manager.h"

#include "input/bindings.h"
#include "input/manager.h"
#include "input/path_input.h"

#include "display/window_log.h"
#include "display/window_title.h"

#include "rpc/parse_commands.h"

#include "control.h"
#include "download.h"
#include "download_list.h"
#include "element_download_list.h"
#include "element_log_complete.h"
#include "element_string_list.h"
#include "root.h"

namespace ui {

DownloadList::DownloadList() :
  m_state(DISPLAY_MAX_SIZE) {

  m_uiArray[DISPLAY_DOWNLOAD]      = NULL;
  m_uiArray[DISPLAY_DOWNLOAD_LIST] = new ElementDownloadList();
  m_uiArray[DISPLAY_LOG]           = new ElementLogComplete(control->core()->log_complete());
  m_uiArray[DISPLAY_STRING_LIST]   = new ElementStringList();

  m_windowLog                      = new WLog(control->core()->log_important());

  setup_keys();
}

DownloadList::~DownloadList() {
  if (is_active())
    throw std::logic_error("ui::DownloadList::~DownloadList() called on an active object");

  std::for_each(m_uiArray, m_uiArray + DISPLAY_MAX_SIZE, rak::call_delete<ElementBase>());

  delete m_windowLog;
}

void
DownloadList::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::DownloadList::activate() called on an already activated object");

  m_frame = frame;

  control->input()->push_back(&m_bindings);

  activate_display(DISPLAY_DOWNLOAD_LIST);
}

void
DownloadList::disable() {
  if (!is_active())
    throw std::logic_error("ui::DownloadList::disable() called on an already disabled object");

  receive_exit_input(INPUT_NONE);
  activate_display(DISPLAY_MAX_SIZE);

  m_frame = NULL;

  control->input()->erase(&m_bindings);
}

core::View*
DownloadList::current_view() {
  return dynamic_cast<ElementDownloadList*>(m_uiArray[DISPLAY_DOWNLOAD_LIST])->view();
}

void
DownloadList::set_current_view(const std::string& name) {
  return dynamic_cast<ElementDownloadList*>(m_uiArray[DISPLAY_DOWNLOAD_LIST])->receive_change_view(name);
}

// This should also do focus_next() or something.
void
DownloadList::unfocus_download(core::Download* d) {
  if (current_view()->focus() >= current_view()->end_visible() || *current_view()->focus() != d)
    return;

  if (m_state == DISPLAY_DOWNLOAD)
    activate_display(DISPLAY_DOWNLOAD_LIST);

  current_view()->next_focus();
}

display::WindowDownloadList*
DownloadList::current_window_list() {
  return dynamic_cast<ElementDownloadList*>(m_uiArray[DISPLAY_DOWNLOAD_LIST])->window();
}

void
DownloadList::activate_display(Display displayType) {
  if (!is_active())
    throw torrent::internal_error("ui::DownloadList::activate_display(...) !is_active().");

  if (displayType > DISPLAY_MAX_SIZE)
    throw torrent::internal_error("ui::DownloadList::activate_display(...) out of bounds");

  if (displayType == m_state)
    return;

  // Cleanup previous state.
  switch (m_state) {
  case DISPLAY_DOWNLOAD:
    m_uiArray[m_state]->disable();

    delete m_uiArray[m_state];
    m_uiArray[m_state] = NULL;

    break;
    
  case DISPLAY_DOWNLOAD_LIST:
    m_uiArray[DISPLAY_DOWNLOAD_LIST]->disable();

    m_windowLog->set_active(false);
    m_frame->frame(1)->clear();

    m_frame->clear();
    break;

  case DISPLAY_LOG:
  case DISPLAY_STRING_LIST:
    m_uiArray[m_state]->disable();
    break;
    
  default:
    break;
  }

  m_state = displayType;

  // Initialize new state.
  switch (displayType) {
  case DISPLAY_DOWNLOAD:
    // If no download has the focus, just return to the download list.
    if (current_view()->focus() == current_view()->end_visible()) {
      m_state = DISPLAY_MAX_SIZE;

      activate_display(DISPLAY_DOWNLOAD_LIST);
      return;

    } else {
      Download* download = new Download(*current_view()->focus());

      download->activate(m_frame);
      download->slot_exit(std::bind(&DownloadList::activate_display, this, DISPLAY_DOWNLOAD_LIST));
    
      m_uiArray[DISPLAY_DOWNLOAD] = download;
      break;
    }

  case DISPLAY_DOWNLOAD_LIST:
    m_frame->initialize_row(2);

    m_uiArray[DISPLAY_DOWNLOAD_LIST]->activate(m_frame->frame(0));

    m_frame->frame(1)->initialize_window(m_windowLog);
    m_windowLog->set_active(true);
    m_windowLog->receive_update();
    break;

  case DISPLAY_LOG:
  case DISPLAY_STRING_LIST:
    m_uiArray[displayType]->activate(m_frame);
    break;

  default:
    break;
  }

  // Set title.
  switch (displayType) {
  case DISPLAY_DOWNLOAD_LIST:
    control->ui()->window_title()->set_title("rTorrent " VERSION "/" +
                                             std::string(torrent::version()) + " - " +
                                             rpc::call_command_string("session.name"));
    break;
  case DISPLAY_LOG:
    control->ui()->window_title()->set_title("Log");
    break;
  default: break;
  }

  control->display()->adjust_layout();
}

void
DownloadList::receive_view_input(Input type) {
  if (control->ui()->current_input() != NULL)
    return;

  input::PathInput* input = new input::PathInput;

  const char* title;

  switch (type) {
  case INPUT_LOAD_DEFAULT:
    title = "load.start";
    break;

  case INPUT_LOAD_MODIFIED:
    title = "load.normal";
    break;

  case INPUT_CHANGE_DIRECTORY:
    title = "change_directory";

    input->str() = rpc::call_command_string("directory.default");
    input->set_pos(input->str().length());

    break;

  case INPUT_COMMAND:
    title = "command";
    break;

  default:
    throw torrent::internal_error("DownloadList::receive_view_input(...) Invalid input type.");
  }

  ElementStringList* esl = dynamic_cast<ElementStringList*>(m_uiArray[DISPLAY_STRING_LIST]);

  input->signal_show_next().push_back(std::bind(&DownloadList::activate_display, this, DISPLAY_STRING_LIST));
  input->signal_show_next().push_back(std::bind(&ElementStringList::next_screen, esl));

  input->signal_show_range().push_back(std::bind(&DownloadList::activate_display, this, DISPLAY_STRING_LIST));
  input->signal_show_range().push_back(std::bind(&ElementStringList::set_range_dirent<utils::Directory::iterator>,
                                                      esl,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2));

  input->bindings()['\n']      = std::bind(&DownloadList::receive_exit_input, this, type);
  input->bindings()[KEY_ENTER] = std::bind(&DownloadList::receive_exit_input, this, type);
  input->bindings()['\x07']    = std::bind(&DownloadList::receive_exit_input, this, INPUT_NONE);

  control->ui()->enable_input(title, input);
}

void
DownloadList::receive_exit_input(Input type) {
  input::TextInput* input = control->ui()->current_input();
  
  // We should check that this object is the one holding the input.
  if (input == NULL)
    return;

  control->ui()->disable_input();

  try {

    switch (type) {
    case INPUT_NONE:
      break;

    case INPUT_LOAD_DEFAULT:
    case INPUT_LOAD_MODIFIED:
      if (input->str().empty())
        break;

      control->core()->try_create_download_expand(input->str(), type == INPUT_LOAD_DEFAULT ? core::Manager::create_start : 0);
      break;

    case INPUT_CHANGE_DIRECTORY:
      if (current_view()->focus() == current_view()->end_visible())
        throw torrent::input_error("No download in focus to change root directory.");

      if ((*current_view()->focus())->is_open())
        throw torrent::input_error("Cannot change root directory on an open download.");

      rpc::call_command("d.directory.set", rak::trim(input->str()), rpc::make_target(*current_view()->focus()));
      control->core()->push_log_std("New root directory \"" + rpc::call_command_string("d.directory", rpc::make_target(*current_view()->focus())) + "\" for torrent.");
      break;

    case INPUT_COMMAND:
      rpc::parse_command_single(current_view()->focus() != current_view()->end_visible() ?
                                rpc::make_target(*current_view()->focus()) : rpc::make_target(),
                                input->str());
      break;

    default:
      throw torrent::internal_error("DownloadList::receive_exit_input(...) Invalid input type.");
    }

  } catch (torrent::input_error& e) {
    lt_log_print(torrent::LOG_WARN, "Input failed: %s", e.what());
  }

  activate_display(DISPLAY_DOWNLOAD_LIST);

  delete input;
}

void
DownloadList::setup_keys() {
  m_bindings['\x7f']        = std::bind(&DownloadList::receive_view_input, this, INPUT_LOAD_DEFAULT);
  m_bindings[KEY_BACKSPACE] = std::bind(&DownloadList::receive_view_input, this, INPUT_LOAD_DEFAULT);
  m_bindings['\n']          = std::bind(&DownloadList::receive_view_input, this, INPUT_LOAD_MODIFIED);
  m_bindings[KEY_ENTER]     = std::bind(&DownloadList::receive_view_input, this, INPUT_LOAD_MODIFIED);
  m_bindings['\x0F']        = std::bind(&DownloadList::receive_view_input, this, INPUT_CHANGE_DIRECTORY);
  m_bindings['X' - '@']     = std::bind(&DownloadList::receive_view_input, this, INPUT_COMMAND);

  m_uiArray[DISPLAY_LOG]->bindings()[KEY_LEFT] =
    m_uiArray[DISPLAY_LOG]->bindings()['B' - '@'] =
    m_uiArray[DISPLAY_LOG]->bindings()[' '] = std::bind(&DownloadList::activate_display, this, DISPLAY_DOWNLOAD_LIST);

  m_uiArray[DISPLAY_DOWNLOAD_LIST]->bindings()[KEY_RIGHT] =
    m_uiArray[DISPLAY_DOWNLOAD_LIST]->bindings()['F' - '@'] = std::bind(&DownloadList::activate_display, this, DISPLAY_DOWNLOAD);
  m_uiArray[DISPLAY_DOWNLOAD_LIST]->bindings()['l'] = std::bind(&DownloadList::activate_display, this, DISPLAY_LOG);
}

}
