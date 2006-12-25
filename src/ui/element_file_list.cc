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

#include <rak/algorithm.h>
#include <torrent/exceptions.h>
#include <torrent/data/file.h>
#include <torrent/data/file_list.h>

#include "display/frame.h"
#include "display/manager.h"
#include "display/window_file_list.h"
#include "input/manager.h"

#include "control.h"
#include "element_file_list.h"

namespace ui {

ElementFileList::ElementFileList(core::Download* d) :
  m_download(d),
  m_window(NULL),
  m_focus(iterator(d->download()->file_list()->begin())) {

  m_bindings[KEY_LEFT] = m_bindings['B' - '@'] = sigc::mem_fun(&m_slotExit, &slot_type::operator());

  m_bindings[' '] = sigc::mem_fun(*this, &ElementFileList::receive_priority);
  m_bindings['*'] = sigc::mem_fun(*this, &ElementFileList::receive_change_all);
  m_bindings[KEY_NPAGE] = sigc::mem_fun(*this, &ElementFileList::receive_pagenext);
  m_bindings[KEY_PPAGE] = sigc::mem_fun(*this, &ElementFileList::receive_pageprev);

  m_bindings[KEY_DOWN] = m_bindings['N' - '@'] = sigc::mem_fun(*this, &ElementFileList::receive_next);
  m_bindings[KEY_UP]   = m_bindings['P' - '@'] = sigc::mem_fun(*this, &ElementFileList::receive_prev);
}

void
ElementFileList::activate(display::Frame* frame, bool focus) {
  if (m_window != NULL)
    throw torrent::internal_error("ui::ElementFileList::activate(...) is_active().");

  if (focus)
    control->input()->push_back(&m_bindings);

  m_window = new WFileList(m_download, &m_focus);
  m_window->set_active(true);

  m_frame = frame;
  m_frame->initialize_window(m_window);

//   control->display()->adjust_layout();
}

void
ElementFileList::disable() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementFileList::disable(...) !is_active().");

  control->input()->erase(&m_bindings);

  m_frame->clear();
  m_frame = NULL;

  delete m_window;
  m_window = NULL;
}

display::Window*
ElementFileList::window() {
  return m_window;
}

void
ElementFileList::receive_next() {
  torrent::FileList* fl = m_download->download()->file_list();

  if (m_focus == iterator(fl->end()) || ++m_focus == iterator(fl->end()))
    m_focus = iterator(fl->begin());

  m_window->mark_dirty();
}

void
ElementFileList::receive_prev() {
  torrent::FileList* fl = m_download->download()->file_list();

  if (m_focus == iterator(fl->begin()))
    m_focus = iterator(fl->end());

  m_focus--;
  m_window->mark_dirty();
}

void
ElementFileList::receive_pagenext() {
  torrent::FileList* fl = m_download->download()->file_list();

  if (m_focus == --iterator(fl->end())) {
    m_focus = iterator(fl->begin());

  } else {
    m_focus = rak::advance_forward(m_focus, iterator(fl->end()), (m_window->height() - 1) / 2);

    if (m_focus == iterator(fl->end()))
      m_focus = --iterator(fl->end());
  }

  m_window->mark_dirty();
}

void
ElementFileList::receive_pageprev() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementFileList::receive_pageprev(...) called on a disabled object");

  torrent::FileList* fl = m_download->download()->file_list();

  if (m_focus == iterator(fl->begin()))
    m_focus = --iterator(fl->end());
  else
    m_focus = rak::advance_backward(m_focus, iterator(fl->begin()), (m_window->height() - 1) / 2);

  m_window->mark_dirty();
}

void
ElementFileList::receive_priority() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementFileList::receive_prev(...) called on a disabled object");

  // Fix priorities.

//   torrent::FileList* fl = m_download->download()->file_list();

//   if (m_focus >= fl->size_files())
//     return;

//   torrent::File* file = *(fl->begin() + m_focus);

//   file->set_priority(next_priority(file->priority()));

  m_download->download()->update_priorities();
  m_window->mark_dirty();
}

void
ElementFileList::receive_change_all() {
  if (m_window == NULL)
    throw torrent::internal_error("ui::ElementFileList::receive_prev(...) called on a disabled object");

//   torrent::FileList* fl = m_download->download()->file_list();

//   if (m_focus >= fl->size_files())
//     return;

//   Priority p = next_priority((*(fl->begin() + m_focus))->priority());

//   for (torrent::FileList::iterator itr = fl->begin(), last = fl->end(); itr != last; ++itr)
//     (*itr)->set_priority(p);

  m_download->download()->update_priorities();
  m_window->mark_dirty();
}

ElementFileList::Priority
ElementFileList::next_priority(Priority p) {
  // Ahh... do +1 modulo.

  switch(p) {
  case torrent::PRIORITY_OFF:
    return torrent::PRIORITY_HIGH;

  case torrent::PRIORITY_NORMAL:
    return torrent::PRIORITY_OFF;

  case torrent::PRIORITY_HIGH:
    return torrent::PRIORITY_NORMAL;
    
  default:
    return torrent::PRIORITY_NORMAL;
  };
}

}
