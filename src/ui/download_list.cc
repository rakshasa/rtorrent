// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <stdexcept>
#include <rak/functional.h>
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <torrent/torrent.h>

#include "core/download.h"

#include "input/bindings.h"
#include "input/path_input.h"

#include "display/window_http_queue.h"
#include "display/window_input.h"
#include "display/window_log.h"
#include "display/window_statusbar.h"
#include "display/window_title.h"

#include "control.h"
#include "download.h"
#include "download_list.h"
#include "element_download_list.h"
#include "element_log_complete.h"
#include "element_string_list.h"

namespace ui {

DownloadList::DownloadList(Control* c) :
  m_state(DISPLAY_MAX_SIZE),

  m_window(c->get_display().end()),

  m_windowTitle(new WTitle("rTorrent " VERSION " - libTorrent " + torrent::get_version())),
  m_windowStatus(new WStatus(&c->get_core())),
  m_windowHttpQueue(new WHttp(&c->get_core().get_http_queue())),

  m_uiDownload(NULL),

  m_downloadList(&c->get_core().get_download_list()),

  m_control(c),
  m_bindings(new input::Bindings)
{
  m_uiArray[DISPLAY_DOWNLOAD_LIST] = new ElementDownloadList(&m_downloadList);
  m_uiArray[DISPLAY_LOG]           = new ElementLogComplete(&m_control->get_core().get_log_complete());
  m_windowLog                      = new WLog(&m_control->get_core().get_log_important());

  m_taskUpdate.set_iterator(utils::taskScheduler.end());
  m_taskUpdate.set_slot(sigc::mem_fun(*this, &DownloadList::task_update)),

  setup_keys();
  setup_input();
}

DownloadList::~DownloadList() {
  if (is_active())
    throw std::logic_error("ui::DownloadList::~DownloadList() called on an active object");

  std::for_each(m_uiArray, m_uiArray + DISPLAY_MAX_SIZE, rak::call_delete<ElementBase>());

  delete m_windowTitle;
  delete m_windowStatus;
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

  utils::taskScheduler.insert(&m_taskUpdate, utils::Timer::cache() + 1000000);

  m_windowTextInput->set_active(false);

  m_window = m_control->get_display().insert(m_control->get_display().begin(), NULL);
  m_control->get_display().push_front(m_windowTitle);

  m_control->get_display().push_back(m_windowLog);
  m_control->get_display().push_back(m_windowHttpQueue);
  m_control->get_display().push_back(m_windowTextInput);
  m_control->get_display().push_back(m_windowStatus);

  m_control->get_input().push_front(m_bindings);

  activate_display(DISPLAY_DOWNLOAD_LIST);
}

void
DownloadList::disable() {
  if (!is_active())
    throw std::logic_error("ui::Download::disable() called on an already disabled object");

  disable_display();

  utils::taskScheduler.erase(&m_taskUpdate);

  if (m_windowTextInput->is_active()) {
    m_windowTextInput->get_input()->clear();
    receive_exit_input();
  }

  m_control->get_display().erase(m_window);
  m_control->get_display().erase(m_windowTitle);
  m_control->get_display().erase(m_windowStatus);
  m_control->get_display().erase(m_windowTextInput);
  m_control->get_display().erase(m_windowLog);
  m_control->get_display().erase(m_windowHttpQueue);

  m_window = m_control->get_display().end();

  m_control->get_input().erase(m_bindings);
}

void
DownloadList::activate_display(Display d) {
  if (!is_active())
    throw std::logic_error("ui::DownloadList::activate_display(...) could not find previous display iterator");

  if (d >= DISPLAY_MAX_SIZE)
    throw std::logic_error("ui::DownloadList::activate_display(...) out of bounds");

  m_state = d;
  m_uiArray[d]->activate(m_control, m_window);

  m_control->get_display().adjust_layout();
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
DownloadList::receive_read_throttle(int t) {
  m_windowStatus->mark_dirty();

  torrent::set_read_throttle(torrent::get_read_throttle() + t * 1024);
}

void
DownloadList::receive_write_throttle(int t) {
  m_windowStatus->mark_dirty();

  torrent::set_write_throttle(torrent::get_write_throttle() + t * 1024);
}

void
DownloadList::receive_start_download() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  m_control->get_core().start(*m_downloadList.get_focus());
}

void
DownloadList::receive_stop_download() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  if ((*m_downloadList.get_focus())->get_download().is_active())
    m_control->get_core().stop(*m_downloadList.get_focus());
  else
    m_downloadList.set_focus(m_control->get_core().erase(m_downloadList.get_focus()));
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

  m_control->get_display().adjust_layout();
}

void
DownloadList::receive_check_hash() {
  if (m_downloadList.get_focus() == m_downloadList.end())
    return;

  m_control->get_core().check_hash(*m_downloadList.get_focus());
}

void
DownloadList::receive_view_input() {
  m_windowStatus->set_active(false);
  m_windowTextInput->set_active(true);
  m_control->get_display().adjust_layout();

  m_control->get_input().set_text_input(m_windowTextInput->get_input());

  m_windowTextInput->set_focus(true);

  (*m_bindings)['\n'] = sigc::mem_fun(*this, &DownloadList::receive_exit_input);
  (*m_bindings)[KEY_ENTER] = sigc::mem_fun(*this, &DownloadList::receive_exit_input);
}

void
DownloadList::receive_exit_input() {
  m_windowStatus->set_active(true);
  m_windowTextInput->set_active(false);

  m_control->get_input().set_text_input();

  m_slotOpenUri(m_windowTextInput->get_input()->str());

  m_windowTextInput->get_input()->clear();
  m_windowTextInput->set_focus(false);

  m_bindings->erase('\n');
  m_bindings->erase(KEY_ENTER);

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
DownloadList::task_update() {
  m_windowLog->receive_update();

  utils::taskScheduler.insert(&m_taskUpdate, (utils::Timer::cache() + 1000000).round_seconds());
}

void
DownloadList::setup_keys() {
  (*m_bindings)['a']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_write_throttle), 1);
  (*m_bindings)['z']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_write_throttle), -1);
  (*m_bindings)['s']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_write_throttle), 5);
  (*m_bindings)['x']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_write_throttle), -5);
  (*m_bindings)['d']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_write_throttle), 50);
  (*m_bindings)['c']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_write_throttle), -50);

  (*m_bindings)['A']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_read_throttle), 1);
  (*m_bindings)['Z']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_read_throttle), -1);
  (*m_bindings)['S']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_read_throttle), 5);
  (*m_bindings)['X']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_read_throttle), -5);
  (*m_bindings)['D']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_read_throttle), 50);
  (*m_bindings)['C']           = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_read_throttle), -50);

  (*m_bindings)['\x13']        = sigc::mem_fun(*this, &DownloadList::receive_start_download);
  (*m_bindings)['\x04']        = sigc::mem_fun(*this, &DownloadList::receive_stop_download);
  (*m_bindings)['\x12']        = sigc::mem_fun(*this, &DownloadList::receive_check_hash);

  (*m_bindings)['\x7f']        = sigc::mem_fun(*this, &DownloadList::receive_view_input);
  (*m_bindings)[KEY_BACKSPACE] = sigc::mem_fun(*this, &DownloadList::receive_view_input);

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
