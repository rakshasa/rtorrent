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

#include <sigc++/bind.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>

#include "core/download.h"
#include "core/manager.h"
#include "core/view.h"
#include "core/view_manager.h"
#include "display/frame.h"
#include "display/manager.h"
#include "input/manager.h"
#include "rpc/parse_commands.h"

#include "control.h"
#include "element_download_list.h"

namespace ui {

ElementDownloadList::ElementDownloadList() :
  m_window(NULL),
  m_view(NULL) {

  receive_change_view("main");

  if (m_view == NULL)
    throw torrent::internal_error("View \"main\" must be present to initialize the main display.");

  m_bindings['\x13']        = sigc::mem_fun(*this, &ElementDownloadList::receive_start_download);
  m_bindings['\x04']        = sigc::mem_fun(*this, &ElementDownloadList::receive_stop_download);
  m_bindings['\x0B']        = sigc::mem_fun(*this, &ElementDownloadList::receive_close_download);
  m_bindings['\x12']        = sigc::mem_fun(*this, &ElementDownloadList::receive_check_hash);
  m_bindings['+']           = sigc::mem_fun(*this, &ElementDownloadList::receive_next_priority);
  m_bindings['-']           = sigc::mem_fun(*this, &ElementDownloadList::receive_prev_priority);
  m_bindings['I']           = sigc::mem_fun(*this, &ElementDownloadList::receive_ignore_ratio);
  m_bindings['U']           = sigc::mem_fun(*this, &ElementDownloadList::receive_clear_tied);

  m_bindings['1']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "main");
  m_bindings['2']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "name");
  m_bindings['3']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "started");
  m_bindings['4']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "stopped");
  m_bindings['5']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "complete");
  m_bindings['6']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "incomplete");
  m_bindings['7']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "hashing");
  m_bindings['8']           = sigc::bind(sigc::mem_fun(*this, &ElementDownloadList::receive_change_view), "seeding");

  m_bindings[KEY_UP]   = m_bindings['P' - '@'] = sigc::mem_fun(*this, &ElementDownloadList::receive_prev);
  m_bindings[KEY_DOWN] = m_bindings['N' - '@'] = sigc::mem_fun(*this, &ElementDownloadList::receive_next);
}

void
ElementDownloadList::activate(display::Frame* frame, bool focus) {
  if (is_active())
    throw torrent::internal_error("ui::ElementDownloadList::activate(...) is_active().");

  control->input()->push_back(&m_bindings);

  m_window = new WDownloadList();
  m_window->set_active(true);
  m_window->set_view(m_view);

  m_frame = frame;
  m_frame->initialize_window(m_window);
}

void
ElementDownloadList::disable() {
  if (!is_active())
    throw torrent::internal_error("ui::ElementDownloadList::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  delete m_window;
  m_window = NULL;
}

void
ElementDownloadList::set_view(core::View* l) {
  m_view = l;
  m_view->sort();

  if (m_window == NULL)
    return;

  m_window->set_view(l);
  m_window->mark_dirty();
}

void
ElementDownloadList::receive_next() {
  m_view->next_focus();
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_prev() {
  m_view->prev_focus();
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_start_download() {
  if (m_view->focus() == m_view->end_visible())
    return;

  control->core()->download_list()->start_normal(*m_view->focus());
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_stop_download() {
  if (m_view->focus() == m_view->end_visible())
    return;

  if (rpc::call_command_d_value("get_d_state", *m_view->focus()) == 1)
    control->core()->download_list()->stop_normal(*m_view->focus());
  else
    control->core()->download_list()->erase(*m_view->focus());

  m_view->set_last_changed();
}

void
ElementDownloadList::receive_close_download() {
  if (m_view->focus() == m_view->end_visible())
    return;

  core::Download* download = *m_view->focus();

  rpc::call_command_d("set_d_ignore_commands", download, (int64_t)1);

  control->core()->download_list()->stop_normal(download);
  control->core()->download_list()->close(download);
  m_view->set_last_changed();
}

void
ElementDownloadList::receive_next_priority() {
  if (m_view->focus() == m_view->end_visible())
    return;

  (*m_view->focus())->set_priority(((*m_view->focus())->priority() + 1) % 4);
  m_window->mark_dirty();
}

void
ElementDownloadList::receive_prev_priority() {
  if (m_view->focus() == m_view->end_visible())
    return;

  (*m_view->focus())->set_priority(((*m_view->focus())->priority() - 1) % 4);
  m_window->mark_dirty();
}

void
ElementDownloadList::receive_check_hash() {
  if (m_view->focus() == m_view->end_visible())
    return;

  // Catch here?
  control->core()->download_list()->check_hash(*m_view->focus());
}

void
ElementDownloadList::receive_ignore_ratio() {
  if (m_view->focus() == m_view->end_visible())
    return;

  if (rpc::call_command_d_value("get_d_ignore_commands", *m_view->focus()) != 0) {
    rpc::call_command_d_set_value("set_d_ignore_commands", *m_view->focus(), (int64_t)0);
    control->core()->push_log("Torrent set to heed commands.");
  } else {
    rpc::call_command_d_set_value("set_d_ignore_commands", *m_view->focus(), (int64_t)1);
    control->core()->push_log("Torrent set to ignore commands.");
  }
}

void
ElementDownloadList::receive_clear_tied() {
  if (m_view->focus() == m_view->end_visible())
    return;

  const std::string& tiedFile = rpc::call_command_d_string("get_d_tied_to_file", *m_view->focus());

  if (!tiedFile.empty()) {
    rpc::call_command_d_void("d_delete_tied", *m_view->focus());

    control->core()->push_log("Cleared tied to file association for the selected download.");
  }
}

void
ElementDownloadList::receive_change_view(const std::string& name) {
  core::ViewManager::iterator itr = control->view_manager()->find(name);

  if (itr == control->view_manager()->end()) {
    control->core()->push_log("Could not find view \"" + name + "\".");
    return;
  }

  set_view(*itr);
}

}
