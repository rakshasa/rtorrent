#include "config.h"

#include <stdexcept>
#include <torrent/torrent.h>
#include <sigc++/bind.h>

#include "core/download.h"

#include "input/bindings.h"
#include "input/text_input.h"

#include "display/window_download_list.h"
#include "display/window_http_queue.h"
#include "display/window_input.h"
#include "display/window_log.h"
#include "display/window_statusbar.h"
#include "display/window_title.h"

#include "control.h"
#include "download.h"
#include "download_list.h"

namespace ui {

DownloadList::DownloadList(Control* c) :
  m_title(new WTitle("rtorrent " VERSION " - " + torrent::get(torrent::LIBRARY_NAME))),
  m_status(new WStatus(&c->get_core())),
  m_textInput(new WInput(new input::TextInput)),
  m_windowHttpQueue(new WHttp(&c->get_core().get_http_queue())),

  m_taskUpdate(sigc::mem_fun(*this, &DownloadList::task_update)),
  m_download(NULL),
  m_focus(c->get_core().get_download_list().end()),
  m_control(c),
  m_bindings(new input::Bindings) {

  m_window = new WList(&m_control->get_core().get_download_list(), &m_focus);
  m_windowLog = new WLog(&m_control->get_core().get_log());

  bind_keys(m_bindings);

  m_textInput->get_input()->slot_dirty(sigc::mem_fun(*m_textInput, &WInput::mark_dirty));
}

DownloadList::~DownloadList() {
  delete m_window;
  delete m_title;
  delete m_status;
  delete m_bindings;

  delete m_windowLog;
  delete m_textInput->get_input();
  delete m_textInput;
  delete m_windowHttpQueue;
}

void
DownloadList::activate() {
  m_taskUpdate.insert(utils::Timer::cache() + 1000000);

  m_textInput->set_active(false);

  m_control->get_input().push_front(m_bindings);

  m_control->get_display().push_back(m_windowLog);
  m_control->get_display().push_back(m_windowHttpQueue);
  m_control->get_display().push_back(m_textInput);
  m_control->get_display().push_back(m_status);
  m_control->get_display().push_front(m_window);
  m_control->get_display().push_front(m_title);
}

void
DownloadList::disable() {
  m_taskUpdate.remove();

  if (m_textInput->is_active()) {
    m_textInput->get_input()->clear();
    receive_exit_input();
  }

  m_control->get_input().erase(m_bindings);

  m_control->get_display().erase(m_title);
  m_control->get_display().erase(m_window);
  m_control->get_display().erase(m_status);
  m_control->get_display().erase(m_textInput);
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
  m_status->mark_dirty();

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

  if (m_download != NULL)
    throw std::logic_error("DownloadList::receive_view_download() called but m_download != NULL");

  disable();

  m_download = new Download(*m_focus, m_control);

  m_download->activate();
  m_download->get_bindings()[KEY_LEFT] = sigc::mem_fun(*this, &DownloadList::receive_exit_download);
}

void
DownloadList::receive_exit_download() {
  if (m_download == NULL)
    throw std::logic_error("DownloadList::receive_exit_download() called but m_download == NULL");

  m_download->disable();
  delete m_download;
  m_download = NULL;

  activate();

  m_control->get_display().adjust_layout();
}

void
DownloadList::receive_view_input() {
  m_status->set_active(false);
  m_textInput->set_active(true);
  m_control->get_display().adjust_layout();

  m_control->get_input().set_text_input(m_textInput->get_input());

  m_textInput->set_focus(true);

  (*m_bindings)['\n'] = sigc::mem_fun(*this, &DownloadList::receive_exit_input);
  (*m_bindings)[KEY_ENTER] = sigc::mem_fun(*this, &DownloadList::receive_exit_input);
}

void
DownloadList::receive_exit_input() {
  m_status->set_active(true);
  m_textInput->set_active(false);
  m_control->get_display().adjust_layout();

  m_control->get_input().set_text_input();

  m_slotOpenUri(m_textInput->get_input()->str());

  m_textInput->get_input()->clear();
  m_textInput->set_focus(false);

  m_bindings->erase('\n');
  m_bindings->erase(KEY_ENTER);
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

  (*b)[KEY_BACKSPACE] = sigc::mem_fun(*this, &DownloadList::receive_view_input);
}

void
DownloadList::mark_dirty() {
  m_window->mark_dirty();
}

}
