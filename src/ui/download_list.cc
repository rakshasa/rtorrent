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
#include <torrent/torrent.h>
#include <sigc++/bind.h>

#include "core/download.h"

#include "input/bindings.h"
#include "input/path_input.h"

#include "display/window_download_list.h"
#include "display/window_http_queue.h"
#include "display/window_input.h"
#include "display/window_log.h"
#include "display/window_log_complete.h"
#include "display/window_statusbar.h"
#include "display/window_title.h"

#include "control.h"
#include "download.h"
#include "download_list.h"

namespace ui {

DownloadList::DownloadList(Control* c) :
  m_windowTitle(new WTitle("rtorrent " VERSION " - " + torrent::get(torrent::LIBRARY_NAME))),
  m_windowStatus(new WStatus(&c->get_core())),
  m_windowTextInput(new WInput(new input::PathInput)),
  m_windowHttpQueue(new WHttp(&c->get_core().get_http_queue())),

  m_taskUpdate(sigc::mem_fun(*this, &DownloadList::task_update)),
  m_uiDownload(NULL),
  m_focus(c->get_core().get_download_list().end()),
  m_control(c),
  m_bindings(new input::Bindings) {

  m_windowDownloadList = new WList(&m_control->get_core().get_download_list(), &m_focus);
  m_windowLog          = new WLog(&m_control->get_core().get_log_important());
  m_windowLogComplete  = NULL;

  bind_keys(m_bindings);

  m_windowTextInput->get_input()->slot_dirty(sigc::mem_fun(*m_windowTextInput, &WInput::mark_dirty));
}

DownloadList::~DownloadList() {
  delete m_windowDownloadList;
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
  m_taskUpdate.insert(utils::Timer::cache() + 1000000);

  m_windowTextInput->set_active(false);

  m_control->get_input().push_front(m_bindings);

  m_control->get_display().push_back(m_windowLog);
  m_control->get_display().push_back(m_windowHttpQueue);
  m_control->get_display().push_back(m_windowTextInput);
  m_control->get_display().push_back(m_windowStatus);
  m_control->get_display().push_front(m_windowDownloadList);
  m_control->get_display().push_front(m_windowTitle);
}

void
DownloadList::disable() {
  m_taskUpdate.remove();

  if (m_windowTextInput->is_active()) {
    m_windowTextInput->get_input()->clear();
    receive_exit_input();
  }

  if (m_windowLogComplete != NULL)
    receive_toggle_log();

  m_control->get_input().erase(m_bindings);

  m_control->get_display().erase(m_windowTitle);
  m_control->get_display().erase(m_windowDownloadList);
  m_control->get_display().erase(m_windowStatus);
  m_control->get_display().erase(m_windowTextInput);
  m_control->get_display().erase(m_windowLog);
  m_control->get_display().erase(m_windowHttpQueue);
}

void
DownloadList::receive_next() {
  if (m_focus != m_control->get_core().get_download_list().end())
    ++m_focus;
  else
    m_focus = m_control->get_core().get_download_list().begin();

  mark_dirty();
}

void
DownloadList::receive_prev() {
  if (m_focus != m_control->get_core().get_download_list().begin())
    --m_focus;
  else
    m_focus = m_control->get_core().get_download_list().end();

  mark_dirty();
}

void
DownloadList::receive_throttle(int t) {
  m_windowStatus->mark_dirty();

  torrent::set(torrent::THROTTLE_ROOT_CONST_RATE, torrent::get(torrent::THROTTLE_ROOT_CONST_RATE) + t * 1024);
}

void
DownloadList::receive_start_download() {
  if (m_focus == m_control->get_core().get_download_list().end())
    return;

  m_control->get_core().start(*m_focus);
}

void
DownloadList::receive_stop_download() {
  if (m_focus == m_control->get_core().get_download_list().end())
    return;

  if ((*m_focus)->get_download().is_active())
    m_control->get_core().stop(*m_focus);
  else
    m_focus = m_control->get_core().erase(m_focus);
}

void
DownloadList::receive_view_download() {
  if (m_focus == m_control->get_core().get_download_list().end())
    return;

  if (m_uiDownload != NULL)
    throw std::logic_error("DownloadList::receive_view_download() called but m_uiDownload != NULL");

  disable();

  m_uiDownload = new Download(*m_focus, m_control);

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
  m_control->get_display().adjust_layout();

  m_control->get_input().set_text_input();

  m_slotOpenUri(m_windowTextInput->get_input()->str());

  m_windowTextInput->get_input()->clear();
  m_windowTextInput->set_focus(false);

  m_bindings->erase('\n');
  m_bindings->erase(KEY_ENTER);
}

void
DownloadList::receive_toggle_log() {
  if (m_windowLogComplete == NULL) {
    display::Manager::iterator itr = m_control->get_display().find(m_windowDownloadList);

    if (itr == m_control->get_display().end())
      throw std::logic_error("ui::DownloadList::receive_toggle_log() could not find download list");

    *itr = m_windowLogComplete = new WLogComplete(&m_control->get_core().get_log_complete());

  } else {
    display::Manager::iterator itr = m_control->get_display().find(m_windowLogComplete);

    if (itr == m_control->get_display().end())
      throw std::logic_error("ui::DownloadList::receive_toggle_log() could not find download list");

    *itr = m_windowDownloadList;

    delete m_windowLogComplete;
    m_windowLogComplete = NULL;
  }

  m_control->get_display().adjust_layout();
}

void
DownloadList::task_update() {
  m_windowLog->receive_update();

  m_taskUpdate.insert(utils::Timer::cache() + 1000000);
}

void
DownloadList::bind_keys(input::Bindings* b) {
  (*b)['a'] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_throttle), 1);
  (*b)['z'] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_throttle), -1);
  (*b)['s'] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_throttle), 5);
  (*b)['x'] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_throttle), -5);
  (*b)['d'] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_throttle), 50);
  (*b)['c'] = sigc::bind(sigc::mem_fun(*this, &DownloadList::receive_throttle), -50);

  (*b)['\x13'] = sigc::mem_fun(*this, &DownloadList::receive_start_download);
  (*b)['\x04'] = sigc::mem_fun(*this, &DownloadList::receive_stop_download);

  (*b)[KEY_UP]    = sigc::mem_fun(*this, &DownloadList::receive_prev);
  (*b)[KEY_DOWN]  = sigc::mem_fun(*this, &DownloadList::receive_next);
  (*b)[KEY_RIGHT] = sigc::mem_fun(*this, &DownloadList::receive_view_download);
  (*b)['l']       = sigc::mem_fun(*this, &DownloadList::receive_toggle_log);

  (*b)['\x7f'] = sigc::mem_fun(*this, &DownloadList::receive_view_input);
  (*b)[KEY_BACKSPACE] = sigc::mem_fun(*this, &DownloadList::receive_view_input);
}

void
DownloadList::mark_dirty() {
  m_windowDownloadList->mark_dirty();
}

}
