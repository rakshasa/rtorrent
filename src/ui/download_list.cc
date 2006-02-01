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
#include <rak/functional.h>
#include <rak/string_manip.h>
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

#include "core/download.h"
#include "core/manager.h"

#include "input/bindings.h"
#include "input/manager.h"
#include "input/path_input.h"

#include "display/window_http_queue.h"
#include "display/window_input.h"
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

DownloadList::DownloadList(Control* c) :
  m_state(DISPLAY_MAX_SIZE),

  m_window(c->display()->end()),

  m_windowTitle(new WTitle("rTorrent " VERSION " - libTorrent " + std::string(torrent::version()))),
  m_windowHttpQueue(new WHttp(&c->core()->http_queue())),

  m_uiDownload(NULL),

  m_downloadList(&c->core()->download_list()),

  m_control(c),
  m_bindings(new input::Bindings)
{
  m_uiArray[DISPLAY_DOWNLOAD_LIST] = new ElementDownloadList(&m_downloadList);
  m_uiArray[DISPLAY_LOG]           = new ElementLogComplete(&m_control->core()->get_log_complete());
  m_windowLog                      = new WLog(&m_control->core()->get_log_important());

  m_taskUpdate.set_slot(rak::mem_fn(this, &DownloadList::task_update)),

  setup_keys();
  setup_input();
}

DownloadList::~DownloadList() {
  if (is_active())
    throw std::logic_error("ui::DownloadList::~DownloadList() called on an active object");

  std::for_each(m_uiArray, m_uiArray + DISPLAY_MAX_SIZE, rak::call_delete<ElementBase>());

  delete m_windowTitle;
  delete m_bindings;

  delete m_windowLog;
  delete m_windowTextInput->get_input();
  delete m_windowTextInput;
  delete m_windowHttpQueue;
}

void
DownloadList::activate() {
  if (is_active())
    throw std::logic_error("ui::Download::activate() called on an already activated object");

  priority_queue_insert(&taskScheduler, &m_taskUpdate, cachedTime);

  m_windowTextInput->set_active(false);

  m_control->display()->push_front(m_windowTextInput);
  m_control->display()->push_front(m_windowHttpQueue);
  m_control->display()->push_front(m_windowLog);
  m_window = m_control->display()->insert(m_control->display()->begin(), NULL);
  m_control->display()->push_front(m_windowTitle);

  m_control->input()->push_front(m_bindings);

  m_control->core()->download_list().slot_map_erase().insert("0_download_list", sigc::mem_fun(this, &DownloadList::receive_download_erased));

  activate_display(DISPLAY_DOWNLOAD_LIST);
}

void
DownloadList::disable() {
  if (!is_active())
    throw std::logic_error("ui::Download::disable() called on an already disabled object");

  if (m_windowTextInput->is_active()) {
    m_windowTextInput->get_input()->clear();
    receive_exit_input(INPUT_NONE);
  }

  disable_display();

  priority_queue_erase(&taskScheduler, &m_taskUpdate);

  m_control->display()->erase(m_window);
  m_control->display()->erase(m_windowTitle);
  m_control->display()->erase(m_windowTextInput);
  m_control->display()->erase(m_windowLog);
  m_control->display()->erase(m_windowHttpQueue);

  m_window = m_control->display()->end();

  m_control->input()->erase(m_bindings);
}

void
DownloadList::activate_display(Display d) {
  if (!is_active())
    throw std::logic_error("ui::DownloadList::activate_display(...) could not find previous display iterator");

  if (d >= DISPLAY_MAX_SIZE)
    throw std::logic_error("ui::DownloadList::activate_display(...) out of bounds");

  m_state = d;
  m_uiArray[d]->activate(m_control, m_window);

  m_control->display()->adjust_layout();
}

// Does not delete disabled window.
void
DownloadList::disable_display() {
  m_uiArray[m_state]->disable(m_control);

  m_state   = DISPLAY_MAX_SIZE;
  *m_window = NULL;
}

void
DownloadList::receive_next() {
  m_downloadList.inc_focus();
}

void
DownloadList::receive_prev() {
  m_downloadList.dec_focus();
}

void
DownloadList::receive_start_download() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  m_control->core()->start(*m_downloadList.get_focus());
}

void
DownloadList::receive_stop_download() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  if ((*m_downloadList.get_focus())->get_download().is_active())
    m_control->core()->stop(*m_downloadList.get_focus());
  else
    m_downloadList.set_focus(m_control->core()->erase(m_downloadList.get_focus()));
}

void
DownloadList::receive_view_download() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  if (m_uiDownload != NULL)
    throw std::logic_error("DownloadList::receive_view_download() called but m_uiDownload != NULL");

  disable();

  m_uiDownload = new Download(*m_downloadList.get_focus(), m_control);

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

  activate();

  m_control->display()->adjust_layout();
}

void
DownloadList::receive_next_priority() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  (*m_downloadList.get_focus())->set_priority(((*m_downloadList.get_focus())->priority() + 1) % 4);
}

void
DownloadList::receive_prev_priority() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  (*m_downloadList.get_focus())->set_priority(((*m_downloadList.get_focus())->priority() - 1) % 4);
}

void
DownloadList::receive_check_hash() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  m_control->core()->check_hash(*m_downloadList.get_focus());
}

void
DownloadList::receive_view_input(Input type) {
  if (m_windowTextInput->get_active())
    return;

  m_control->ui()->window_statusbar()->set_active(false);
  m_windowTextInput->set_active(true);
  m_control->display()->adjust_layout();

  m_control->input()->set_text_input(m_windowTextInput->get_input());

  m_windowTextInput->set_focus(true);

  if (type == INPUT_CHANGE_DIRECTORY) {
    m_windowTextInput->get_input()->str() = m_control->variables()->get_string("directory");
    m_windowTextInput->get_input()->set_pos(m_windowTextInput->get_input()->str().length());
  }

  (*m_bindings)['\n']      = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_exit_input), type);
  (*m_bindings)[KEY_ENTER] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_exit_input), type);
  (*m_bindings)['\x07']    = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_exit_input), INPUT_NONE);
}

void
DownloadList::receive_exit_input(Input type) {
  if (!m_windowTextInput->get_active())
    return;

  m_control->ui()->window_statusbar()->set_active(true);
  m_windowTextInput->set_active(false);
  m_control->input()->set_text_input();
    
  try {

    switch (type) {
    case INPUT_NONE:
      break;

    case INPUT_LOAD_DEFAULT:
    case INPUT_LOAD_MODIFIED:
      m_control->core()->try_create_download_expand(m_windowTextInput->get_input()->str(), type == INPUT_LOAD_DEFAULT);
      break;

    case INPUT_CHANGE_DIRECTORY:
      if (m_downloadList.get_focus() == m_downloadList.end())
	throw torrent::input_error("No download in focus to change root directory.");

      (*m_downloadList.get_focus())->variables()->set("directory", rak::trim(m_windowTextInput->get_input()->str()));
      m_control->core()->push_log("New root dir \"" + (*m_downloadList.get_focus())->variables()->get_string("directory") + "\"");
      break;

    case INPUT_COMMAND:
      m_control->variables()->process_command(m_windowTextInput->get_input()->str());
      break;
    }

  } catch (torrent::input_error& e) {
    m_control->core()->push_log(e.what());
  }

  // Clean up.
  m_windowTextInput->get_input()->clear();
  m_windowTextInput->set_focus(false);

  m_bindings->erase('\n');
  m_bindings->erase(KEY_ENTER);

  // Urgh... this is ugly...
  (*m_bindings)['\n']          = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_MODIFIED);
  (*m_bindings)[KEY_ENTER]     = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_MODIFIED);

  receive_change(DISPLAY_DOWNLOAD_LIST);
}

void
DownloadList::receive_change(Display d) {
  if (d == m_state)
    return;

  disable_display();
  activate_display(d);
}

void
DownloadList::receive_download_erased(core::Download* d) {
  if (m_downloadList.get_focus() == m_downloadList.end() ||
      *m_downloadList.get_focus() != d)
    return;

  if (m_uiDownload != NULL)
    receive_exit_download();

  receive_next();
}

void
DownloadList::task_update() {
  m_windowLog->receive_update();

  priority_queue_insert(&taskScheduler, &m_taskUpdate, (cachedTime + 1000000).round_seconds());
}

void
DownloadList::setup_keys() {
  (*m_bindings)['\x13']        = sigc::mem_fun(*this, &DownloadList::receive_start_download);
  (*m_bindings)['\x04']        = sigc::mem_fun(*this, &DownloadList::receive_stop_download);
  (*m_bindings)['\x12']        = sigc::mem_fun(*this, &DownloadList::receive_check_hash);
  (*m_bindings)['+']           = sigc::mem_fun(*this, &DownloadList::receive_next_priority);
  (*m_bindings)['-']           = sigc::mem_fun(*this, &DownloadList::receive_prev_priority);

  (*m_bindings)['\x7f']        = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_DEFAULT);
  (*m_bindings)[KEY_BACKSPACE] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_DEFAULT);
  (*m_bindings)['\n']          = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_MODIFIED);
  (*m_bindings)[KEY_ENTER]     = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_LOAD_MODIFIED);
  (*m_bindings)['\x0F']        = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_CHANGE_DIRECTORY);
  (*m_bindings)['\x10']        = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_view_input), INPUT_COMMAND);

  (*m_bindings)[KEY_UP]        = sigc::mem_fun(*this, &DownloadList::receive_prev);
  (*m_bindings)[KEY_DOWN]      = sigc::mem_fun(*this, &DownloadList::receive_next);
  (*m_bindings)[KEY_RIGHT]     = sigc::mem_fun(*this, &DownloadList::receive_view_download);
  (*m_bindings)['l']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change), DISPLAY_LOG);

  m_uiArray[DISPLAY_LOG]->get_bindings()[' '] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change), DISPLAY_DOWNLOAD_LIST);
}

void
DownloadList::setup_input() {
  input::PathInput* p    = new input::PathInput;
  ElementStringList* esl = new ElementStringList();
  m_windowTextInput      = new WInput(p);

  p->slot_dirty(sigc::mem_fun(*m_windowTextInput, &WInput::mark_dirty));

  p->signal_show_next().connect(sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change), DISPLAY_STRING_LIST));
  p->signal_show_next().connect(sigc::mem_fun(*esl, &ElementStringList::next_screen));

  p->signal_show_range().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_change), DISPLAY_STRING_LIST))));
  p->signal_show_range().connect(sigc::mem_fun(*esl, &ElementStringList::set_range<utils::Directory::iterator>));

  m_uiArray[DISPLAY_STRING_LIST] = esl;
}

}
