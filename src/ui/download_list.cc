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

#include <rak/functional.h>
#include <rak/string_manip.h>
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

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
#include "display/window_statusbar.h"

#include "control.h"
#include "download.h"
#include "download_list.h"
#include "element_download_list.h"
#include "element_log_complete.h"
#include "element_string_list.h"
#include "root.h"

namespace ui {

DownloadList::DownloadList() :
  m_state(DISPLAY_NONE),

  m_uiDownload(NULL) {

  m_uiArray[DISPLAY_DOWNLOAD_LIST] = new ElementDownloadList();
  m_uiArray[DISPLAY_LOG]           = new ElementLogComplete(&control->core()->get_log_complete());
  m_uiArray[DISPLAY_STRING_LIST]   = new ElementStringList();

  m_windowLog                      = new WLog(&control->core()->get_log_important());

  receive_change_view("main");

  if (m_view == NULL)
    throw torrent::client_error("View \"main\" must be present to initialize the main display.");

  m_taskUpdate.set_slot(rak::mem_fn(this, &DownloadList::task_update)),

  setup_keys();
}

DownloadList::~DownloadList() {
  if (is_active())
    throw std::logic_error("ui::DownloadList::~DownloadList() called on an active object");

  std::for_each(m_uiArray, m_uiArray + DISPLAY_MAX_SIZE, rak::call_delete<ElementBase>());

  delete m_windowLog;
}

void
DownloadList::activate(display::Frame* frame) {
  if (is_active())
    throw torrent::client_error("ui::DownloadList::activate() called on an already activated object");

  priority_queue_insert(&taskScheduler, &m_taskUpdate, cachedTime);

  m_frame = frame;

  control->input()->push_front(&m_bindings);
  control->core()->download_list()->slot_map_erase()["0_download_list"] = sigc::mem_fun(this, &DownloadList::receive_download_erased);

  activate_display(DISPLAY_DOWNLOAD_LIST);
}

void
DownloadList::disable() {
  if (!is_active())
    throw std::logic_error("ui::DownloadList::disable() called on an already disabled object");

  receive_exit_input(INPUT_NONE);
  activate_display(DISPLAY_NONE);

  m_frame = NULL;

  priority_queue_erase(&taskScheduler, &m_taskUpdate);
  control->input()->erase(&m_bindings);
}

void
DownloadList::activate_display(Display displayType) {
  if (!is_active())
    throw torrent::client_error("ui::DownloadList::activate_display(...) !is_active().");

  if (displayType >= DISPLAY_MAX_SIZE)
    throw torrent::client_error("ui::DownloadList::activate_display(...) out of bounds");

  if (displayType == m_state)
    return;

  // Don't use default.

  // Cleanup previous state.
  switch (m_state) {
  case DISPLAY_DOWNLOAD_LIST:
    m_uiArray[DISPLAY_DOWNLOAD_LIST]->disable();
    m_uiArray[DISPLAY_LOG]->disable();
    m_frame->clear();
    break;

  case DISPLAY_LOG:
    m_uiArray[m_state]->disable();
    break;
    
  default:
    break;
  }

  m_state = displayType;

  // Initialize new state.
  switch (displayType) {
  case DISPLAY_NONE:
    break;

  case DISPLAY_DOWNLOAD_LIST:
    m_frame->initialize_column(2);

    m_uiArray[DISPLAY_DOWNLOAD_LIST]->activate(m_frame->frame(0));
    m_uiArray[DISPLAY_LOG]->activate(m_frame->frame(1));
    break;

  case DISPLAY_LOG:
    m_uiArray[displayType]->activate(m_frame);
    break;

  default:
    break;
  }

  // Set title.
  switch (displayType) {
  case DISPLAY_DOWNLOAD_LIST: control->ui()->window_title()->set_title("rTorrent " VERSION " - libTorrent " + std::string(torrent::version())); break;
  case DISPLAY_LOG:           control->ui()->window_title()->set_title("Log"); break;
  default: break;
  }

  control->display()->adjust_layout();
}

display::Window*
DownloadList::window() {
  return NULL;
}

void
DownloadList::receive_next() {
  m_view->next_focus();
  m_view->set_last_changed();
}

void
DownloadList::receive_prev() {
  m_view->prev_focus();
  m_view->set_last_changed();
}

void
DownloadList::receive_start_download() {
  if (m_view->focus() == m_view->end_visible())
    return;

  control->core()->download_list()->start_normal(*m_view->focus());
  m_view->set_last_changed();
}

void
DownloadList::receive_stop_download() {
  if (m_view->focus() == m_view->end_visible())
    return;

  if ((*m_view->focus())->variable()->get_value("state") == 1)
    control->core()->download_list()->stop_normal(*m_view->focus());
  else
    control->core()->download_list()->erase(*m_view->focus());

  m_view->set_last_changed();
}

void
DownloadList::receive_close_download() {
  if (m_view->focus() == m_view->end_visible())
    return;

  core::Download* download = *m_view->focus();

  control->core()->download_list()->stop_normal(download);
  control->core()->download_list()->close(download);
  m_view->set_last_changed();
}

void
DownloadList::receive_view_download() {
  if (m_view->focus() == m_view->end_visible())
    return;

  if (m_uiDownload != NULL)
    throw std::logic_error("DownloadList::receive_view_download() called but m_uiDownload != NULL");

  disable();

  m_uiDownload = new Download(*m_view->focus(), control);

  m_uiDownload->activate();
  m_uiDownload->get_bindings()[KEY_LEFT] = sigc::mem_fun(*this, &DownloadList::receive_exit_download);
}

void
DownloadList::receive_exit_download() {
  if (m_uiDownload == NULL)
    throw std::logic_error("DownloadList::receive_exit_download() called but m_uiDownload == NULL");

  m_uiDownload->disable();
  delete m_uiDownload;
  m_uiDownload = NULL;

  m_view->set_last_changed();
  activate(m_frame);

  control->display()->adjust_layout();
}

void
DownloadList::receive_next_priority() {
  if (m_view->focus() == m_view->end_visible())
    return;

  (*m_view->focus())->set_priority(((*m_view->focus())->priority() + 1) % 4);
//   (*m_window)->mark_dirty();
}

void
DownloadList::receive_prev_priority() {
  if (m_view->focus() == m_view->end_visible())
    return;

  (*m_view->focus())->set_priority(((*m_view->focus())->priority() - 1) % 4);
//   (*m_window)->mark_dirty();
}

void
DownloadList::receive_check_hash() {
  if (m_view->focus() == m_view->end_visible())
    return;

  // Catch here?
  control->core()->download_list()->check_hash(*m_view->focus());
}

void
DownloadList::receive_ignore_ratio() {
  if (m_view->focus() == m_view->end_visible())
    return;

  if ((*m_view->focus())->variable()->get_value("ignore_commands") != 0) {
    (*m_view->focus())->variable()->set("ignore_commands", (int64_t)0);
    control->core()->push_log("Torrent set to heed commands.");
  } else {
    (*m_view->focus())->variable()->set("ignore_commands", (int64_t)1);
    control->core()->push_log("Torrent set to ignore commands.");
  }
}

void
DownloadList::receive_clear_tied() {
  if (m_view->focus() == m_view->end_visible())
    return;

  const std::string& tiedFile = (*m_view->focus())->variable()->get_string("tied_to_file");

  if (!tiedFile.empty()) {
    // Move this into core?
    control->core()->delete_tied(*m_view->focus());

    control->core()->push_log("Cleared tied to file association for download.");
  }
}

void
DownloadList::receive_view_input(Input type) {
  if (control->ui()->current_input() != NULL)
    return;

  input::PathInput* input = new input::PathInput;

  const char* title;

  switch (type) {
  case INPUT_LOAD_DEFAULT:
    title = "load_start";
    break;

  case INPUT_LOAD_MODIFIED:
    title = "load";
    break;

  case INPUT_CHANGE_DIRECTORY:
    title = "change_directory";

    input->str() = control->variable()->get_string("directory");
    input->set_pos(input->str().length());

    break;

  case INPUT_COMMAND:
    title = "command";
    break;

  default:
    throw torrent::client_error("DownloadList::receive_view_input(...) Invalid input type.");
  }

  input->bindings()['\n']      = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_exit_input), type);
  input->bindings()[KEY_ENTER] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_exit_input), type);
  input->bindings()['\x07']    = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_exit_input), INPUT_NONE);

  m_bindings.disable();
  control->ui()->enable_input(title, input);
}

void
DownloadList::receive_exit_input(Input type) {
  input::TextInput* input = control->ui()->current_input();
  
  // We should check that this object is the one holding the input.
  if (input == NULL)
    return;

  control->ui()->disable_input();
  m_bindings.enable();

  try {

    switch (type) {
    case INPUT_NONE:
      break;

    case INPUT_LOAD_DEFAULT:
    case INPUT_LOAD_MODIFIED:
      control->core()->try_create_download_expand(input->str(), type == INPUT_LOAD_DEFAULT);
      break;

    case INPUT_CHANGE_DIRECTORY:
      if (m_view->focus() == m_view->end_visible())
        throw torrent::input_error("No download in focus to change root directory.");

      (*m_view->focus())->variable()->set("directory", rak::trim(input->str()));
      control->core()->push_log("New root directory \"" + (*m_view->focus())->variable()->get_string("directory") + "\" for torrent.");
      break;

    case INPUT_COMMAND:
      control->variable()->process_command(input->str());
      break;

    default:
      throw torrent::client_error("DownloadList::receive_exit_input(...) Invalid input type.");
    }

  } catch (torrent::input_error& e) {
    control->core()->push_log(e.what());
  }

  delete input;
}

void
DownloadList::receive_download_erased(core::Download* d) {
  if (m_view->focus() == m_view->end_visible() || *m_view->focus() != d)
    return;

  if (m_uiDownload != NULL)
    receive_exit_download();

  receive_next();
}

void
DownloadList::receive_change_view(const std::string& name) {
  core::ViewManager::iterator itr = control->view_manager()->find(name);

  if (itr == control->view_manager()->end()) {
    control->core()->push_log("Could not find view \"" + name + "\".");
    return;
  }

  m_view = *itr;
  m_view->sort();

  ElementDownloadList* ui = dynamic_cast<ElementDownloadList*>(m_uiArray[DISPLAY_DOWNLOAD_LIST]);

  if (ui == NULL)
    throw torrent::client_error("DownloadList::receive_change_view(...) could not cast ui.");

  ui->set_view(m_view);
}

void
DownloadList::task_update() {
  m_windowLog->receive_update();

  priority_queue_insert(&taskScheduler, &m_taskUpdate, (cachedTime + rak::timer::from_seconds(1)).round_seconds());
}

void
DownloadList::setup_keys() {
  m_bindings['\x13']        = sigc::mem_fun(*this, &DownloadList::receive_start_download);
  m_bindings['\x04']        = sigc::mem_fun(*this, &DownloadList::receive_stop_download);
  m_bindings['\x0B']        = sigc::mem_fun(*this, &DownloadList::receive_close_download);
  m_bindings['\x12']        = sigc::mem_fun(*this, &DownloadList::receive_check_hash);
  m_bindings['+']           = sigc::mem_fun(*this, &DownloadList::receive_next_priority);
  m_bindings['-']           = sigc::mem_fun(*this, &DownloadList::receive_prev_priority);
  m_bindings['I']           = sigc::mem_fun(*this, &DownloadList::receive_ignore_ratio);
  m_bindings['U']           = sigc::mem_fun(*this, &DownloadList::receive_clear_tied);

  m_bindings['\x7f']        = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_DEFAULT);
  m_bindings[KEY_BACKSPACE] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_DEFAULT);
  m_bindings['\n']          = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_MODIFIED);
  m_bindings[KEY_ENTER]     = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_MODIFIED);
  m_bindings['\x0F']        = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_CHANGE_DIRECTORY);
  m_bindings['\x10']        = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_COMMAND);

  m_bindings[KEY_UP]        = sigc::mem_fun(*this, &DownloadList::receive_prev);
  m_bindings[KEY_DOWN]      = sigc::mem_fun(*this, &DownloadList::receive_next);
  m_bindings[KEY_RIGHT]     = sigc::mem_fun(*this, &DownloadList::receive_view_download);
  m_bindings['l']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::activate_display), DISPLAY_LOG);

  m_bindings['1']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change_view), "main");
  m_bindings['2']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change_view), "name");
  m_bindings['3']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change_view), "started");
  m_bindings['4']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change_view), "stopped");
  m_bindings['5']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change_view), "complete");
  m_bindings['6']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change_view), "incomplete");
  m_bindings['7']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change_view), "hashing");

  m_uiArray[DISPLAY_LOG]->bindings()[' ']      = sigc::bind(sigc::mem_fun(*this, &DownloadList::activate_display), DISPLAY_DOWNLOAD_LIST);
  m_uiArray[DISPLAY_LOG]->bindings()[KEY_LEFT] = sigc::bind(sigc::mem_fun(*this, &DownloadList::activate_display), DISPLAY_DOWNLOAD_LIST);
}

// void
// DownloadList::setup_input() {
//   input::PathInput* p    = new input::PathInput;
//   m_windowTextInput      = new WInput(p);

//   p->slot_dirty(sigc::mem_fun(*m_windowTextInput, &WInput::mark_dirty));

//   p->signal_show_next().connect(sigc::bind(sigc::mem_fun(*this, &DownloadList::activate_display), DISPLAY_STRING_LIST));
//   p->signal_show_next().connect(sigc::mem_fun(*esl, &ElementStringList::next_screen));

//   p->signal_show_range().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(*this, &DownloadList::activate_display), DISPLAY_STRING_LIST))));
//   p->signal_show_range().connect(sigc::mem_fun(*esl, &ElementStringList::set_range<utils::Directory::iterator>));

// }

}
