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

#ifndef RTORRENT_DISPLAY_MANAGER_H
#define RTORRENT_DISPLAY_MANAGER_H

#include <list>
#include <rak/priority_queue_default.h>

namespace display {

class Window;

class Manager : private std::list<Window*> {
public:
  typedef std::list<Window*> Base;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::push_front;
  using Base::push_back;

  Manager();
  ~Manager();

  void                force_redraw();

  iterator            insert(iterator pos, Window* w);
  iterator            erase(iterator pos);
  iterator            erase(Window* w);

  iterator            find(Window* w);

  void                schedule(Window* w, rak::timer t);
  void                unschedule(Window* w);

  void                adjust_layout();

private:
  void                receive_update();

  void                schedule_update();

  bool                m_forceRedraw;
  rak::timer          m_timeLastUpdate;

  rak::priority_queue_default m_scheduler;
  rak::priority_item          m_taskUpdate;
};

}

#endif
