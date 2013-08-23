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

#include <rak/algorithm.h>
#include <torrent/exceptions.h>
#include <torrent/data/file.h>
#include <torrent/data/file_list.h>

#include "display/frame.h"
#include "display/manager.h"
#include "display/text_element_string.h"
#include "display/window_file_list.h"
#include "input/manager.h"

#include "control.h"
#include "element_file_list.h"
#include "element_text.h"

namespace ui {

ElementFileList::ElementFileList(core::Download* d) :
  m_download(d),

  m_state(DISPLAY_MAX_SIZE),
  m_window(NULL),
  m_elementInfo(NULL),

  m_selected(iterator(d->download()->file_list()->begin())),
  m_collapsed(false) {

  m_bindings[KEY_LEFT]  = m_bindings['B' - '@'] = std::tr1::bind(&slot_type::operator(), &m_slot_exit);
  m_bindings[KEY_RIGHT] = m_bindings['F' - '@'] = std::tr1::bind(&ElementFileList::receive_select, this);

  m_bindings[' '] = std::tr1::bind(&ElementFileList::receive_priority, this);
  m_bindings['*'] = std::tr1::bind(&ElementFileList::receive_change_all, this);
  m_bindings['/'] = std::tr1::bind(&ElementFileList::receive_collapse, this);
  m_bindings[KEY_NPAGE] = std::tr1::bind(&ElementFileList::receive_pagenext, this);
  m_bindings[KEY_PPAGE] = std::tr1::bind(&ElementFileList::receive_pageprev, this);

  m_bindings[KEY_DOWN] = m_bindings['N' - '@'] = std::tr1::bind(&ElementFileList::receive_next, this);
  m_bindings[KEY_UP]   = m_bindings['P' - '@'] = std::tr1::bind(&ElementFileList::receive_prev, this);
}

inline ElementText*
element_file_list_create_info() {
  using namespace display::helpers;

  ElementText* element = new ElementText(rpc::make_target());

  element->set_column(1);
  element->set_interval(1);

  element->push_back("File info:");
  element->push_back("");
  
  element->push_column("Filename:", te_command("fi.filename_last="));
  element->push_back("");
  
  element->push_column("Size:",   te_command("if=$fi.is_file=,$convert.xb=$f.size_bytes=,---"));
  element->push_column("Chunks:", te_command("cat=$f.completed_chunks=,\" / \",$f.size_chunks="));
  element->push_column("Range:",  te_command("cat=$f.range_first=,\" - \",$f.range_second="));
  element->push_back("");

  element->push_column("Queued:",     te_command("cat=\"$if=$f.is_create_queued=,create\",\" \",\"$if=$f.is_resize_queued=,resize\""));
  element->push_column("Prioritize:", te_command("cat=\"$if=$f.prioritize_first=,first\",\" \",\"$if=$f.prioritize_last=,last\""));

  element->set_column_width(element->column_width() + 1);

  return element;
}

void
ElementFileList::activate(display::Frame* frame, bool focus) {
  if (m_window != NULL)
    throw torrent::internal_error("ui::ElementFileList::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_window = new WFileList(this);
  m_window->set_active(true);
  m_window->set_focused(focus);

  m_elementInfo = element_file_list_create_info();
  m_elementInfo->slot_exit(std::tr1::bind(&ElementFileList::activate_display, this, DISPLAY_LIST));
  m_elementInfo->set_target(rpc::make_target(&m_selected));

  m_frame = frame;

  activate_display(DISPLAY_LIST);
}

void
ElementFileList::disable() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementFileList::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  activate_display(DISPLAY_MAX_SIZE);

  m_frame->clear();
  m_frame = NULL;

  delete m_window;
  m_window = NULL;

  delete m_elementInfo;
  m_elementInfo = NULL;
}

void
ElementFileList::activate_display(Display display) {
  if (display == m_state)
    return;

  switch (m_state) {
  case DISPLAY_INFO:
    m_elementInfo->disable();
    break;

  case DISPLAY_LIST:
    m_window->set_active(false);
    m_frame->clear();
    break;

  case DISPLAY_MAX_SIZE:
    break;
  }

  m_state = display;

  switch (m_state) {
  case DISPLAY_INFO:
    m_elementInfo->activate(m_frame, true);
    break;

  case DISPLAY_LIST:
    m_window->set_active(true);
    m_frame->initialize_window(m_window);
    break;

  case DISPLAY_MAX_SIZE:
    break;
  }

  control->display()->adjust_layout();
}

void
ElementFileList::receive_next() {
  torrent::FileList* fl = m_download->download()->file_list();

  if (is_collapsed())
    m_selected.forward_current_depth();
  else
    m_selected++;

  if (m_selected == iterator(fl->end()))
    m_selected = iterator(fl->begin());

  update_itr();
}

void
ElementFileList::receive_prev() {
  torrent::FileList* fl = m_download->download()->file_list();

  if (m_selected == iterator(fl->begin()))
    m_selected = iterator(fl->end());

  if (is_collapsed())
    m_selected.backward_current_depth();
  else
    m_selected--;

  update_itr();
}

void
ElementFileList::receive_pagenext() {
  torrent::FileList* fl = m_download->download()->file_list();

  if (m_selected == --iterator(fl->end())) {
    m_selected = iterator(fl->begin());

  } else {
    m_selected = rak::advance_forward(m_selected, iterator(fl->end()), (m_window->height() - 1) / 2);

    if (m_selected == iterator(fl->end()))
      m_selected = --iterator(fl->end());
  }

  update_itr();
}

void
ElementFileList::receive_pageprev() {
  if (m_window == NULL)
    return;

  torrent::FileList* fl = m_download->download()->file_list();

  if (m_selected == iterator(fl->begin()))
    m_selected = --iterator(fl->end());
  else
    m_selected = rak::advance_backward(m_selected, iterator(fl->begin()), (m_window->height() - 1) / 2);

  update_itr();
}

void
ElementFileList::receive_select() {
  if (m_window == NULL || m_state != DISPLAY_LIST)
    return;

  if (is_collapsed() && !m_selected.is_file()) {
    torrent::FileList* fl = m_download->download()->file_list();
    m_selected++;
    if (m_selected == iterator(fl->end()))
      m_selected = iterator(fl->begin());
    m_window->mark_dirty();
  } else {
    activate_display(DISPLAY_INFO);
  }
}

void
ElementFileList::receive_priority() {
  if (m_window == NULL)
    return;

  torrent::priority_t priority = torrent::priority_t((m_selected.file()->priority() + 2) % 3);

  iterator first = m_selected; 
  iterator last = m_selected;
  last.forward_current_depth();

  while (first != last) {
    if (first.is_file())
      first.file()->set_priority(priority);

    first++;
  }

  m_download->download()->update_priorities();
  update_itr();
}

void
ElementFileList::receive_change_all() {
  if (m_window == NULL)
    return;

  torrent::FileList* fl = m_download->download()->file_list();
  torrent::priority_t priority = torrent::priority_t((m_selected.file()->priority() + 2) % 3);

  for (torrent::FileList::iterator itr = fl->begin(), last = fl->end(); itr != last; ++itr)
    (*itr)->set_priority(priority);

  m_download->download()->update_priorities();
  update_itr();
}

void
ElementFileList::receive_collapse() {
  if (m_window == NULL)
    return;

  set_collapsed(!is_collapsed());
  m_window->mark_dirty();
}

void
ElementFileList::update_itr() {
  m_window->mark_dirty();
  m_elementInfo->mark_dirty();
}

}
