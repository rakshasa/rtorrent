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

#ifndef RTORRENT_CORE_LOG_H
#define RTORRENT_CORE_LOG_H

#include <deque>
#include <string>
#include <sigc++/signal.h>

#include "utils/timer.h"

namespace core {

class Log : private std::deque<std::pair<utils::Timer, std::string> > {
public:
  typedef std::pair<utils::Timer, std::string> Type;
  typedef std::deque<Type>                     Base;
  typedef sigc::signal0<void>                  Signal;

  using Base::iterator;
  using Base::const_iterator;
  using Base::reverse_iterator;
  using Base::const_reverse_iterator;

  using Base::begin;
  using Base::end;
  using Base::rbegin;
  using Base::rend;

  using Base::empty;
  using Base::size;

  void      push_front(const std::string& msg);

  iterator  find_older(utils::Timer t);

  Signal&   signal_update() { return m_signalUpdate; }

private:
  Signal    m_signalUpdate;
};

}

#endif
