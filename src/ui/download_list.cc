#include "config.h"

#include <stdexcept>
#include <torrent/torrent.h>

#include "input/bindings.h"
#include "display/window_title.h"
#include "display/window_download_list.h"

#include "control.h"
#include "download.h"
#include "download_list.h"

namespace ui {

DownloadList::DownloadList(core::DownloadList* l, Control* c) :
  m_window(new WList(l)),
  m_title(new WTitle("rtorrent " VERSION " - " + torrent::get(torrent::LIBRARY_NAME))),
  m_download(NULL),
  m_control(c),
  m_bindings(new input::Bindings) {

  (*m_bindings)[KEY_UP]    = sigc::mem_fun(*this, &DownloadList::receive_prev);
  (*m_bindings)[KEY_DOWN]  = sigc::mem_fun(*this, &DownloadList::receive_next);
  (*m_bindings)[KEY_RIGHT] = sigc::mem_fun(*this, &DownloadList::receive_view_download);
}

DownloadList::~DownloadList() {
  delete m_window;
  delete m_title;
  delete m_bindings;
}

void
DownloadList::activate() {
  m_control->get_display().push_front(m_window);
  m_control->get_display().push_front(m_title);
  m_control->get_input().push_front(m_bindings);
}

void
DownloadList::disable() {
  m_control->get_display().erase(m_title);
  m_control->get_display().erase(m_window);
  m_control->get_input().erase(m_bindings);
}

void
DownloadList::receive_next() {
  if (m_window->get_focus() == m_window->get_list().end())
    m_window->set_focus(m_window->get_list().begin());
  else
    m_window->set_focus(++m_window->get_focus());
}

void
DownloadList::receive_prev() {
  if (m_window->get_focus() == m_window->get_list().begin())
    m_window->set_focus(m_window->get_list().end());
  else
    m_window->set_focus(--m_window->get_focus());
}

void
DownloadList::receive_view_download() {
  if (m_window->get_focus() == m_window->get_list().end())
    return;

  disable();

  m_download = new Download(m_window->get_focus(), m_control);

  m_download->get_bindings()[KEY_LEFT] = sigc::mem_fun(*this, &DownloadList::receive_exit_download);
  m_download->activate();

  m_control->get_display().adjust_layout();
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

}
