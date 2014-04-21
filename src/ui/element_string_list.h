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

#ifndef RTORRENT_UI_ELEMENT_STRING_LIST_H
#define RTORRENT_UI_ELEMENT_STRING_LIST_H

#include <list>
#include <string>
#include <algorithm>
#include <functional>
#include <torrent/utils/log.h>

#include "element_base.h"

#include "display/window_string_list.h"

class Control;

namespace ui {

class ElementStringList : public ElementBase {
public:
  typedef display::WindowStringList     WStringList;
  typedef std::list<std::string>        List;

  ElementStringList();

  void                activate(display::Frame* frame, bool focus = true);
  void                disable();

  display::Window*    window() { return m_window; }

  template <typename InputIter>
  void                set_range(InputIter first, InputIter last) {
    m_list.clear();

    while (first != last)
      m_list.push_back(*(first++));

    if (m_window != NULL) {
      lt_log_print(torrent::LOG_UI_EVENTS, "element_string_list: set range (visible)");

      m_window->set_range(m_list.begin(), m_list.end());
      m_window->mark_dirty();
    } else {
      lt_log_print(torrent::LOG_UI_EVENTS, "element_string_list: set range (hidden)");
    }
  }

  // A hack, clean this up.
  template <typename InputIter>
  void                set_range_dirent(InputIter first, InputIter last) {
    m_list.clear();

    while (first != last)
      m_list.push_back((first++)->d_name);

    if (m_window != NULL) {
      lt_log_print(torrent::LOG_UI_EVENTS, "element_string_list: set dirent range (visible)");

      m_window->set_range(m_list.begin(), m_list.end());
      m_window->mark_dirty();
    } else {
      lt_log_print(torrent::LOG_UI_EVENTS, "element_string_list: set dirent range (hidden)");
    }
  }

  void                next_screen();

private:
  WStringList*        m_window;
  List                m_list;
};

}

#endif
