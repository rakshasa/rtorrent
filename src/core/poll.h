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

#ifndef RTORRENT_CORE_POLL_H
#define RTORRENT_CORE_POLL_H

#include <sys/select.h>
#include <sigc++/slot.h>

#include "utils/timer.h"
#include "curl_stack.h"

namespace core {

class CurlGet;

class Poll {
public:
  typedef sigc::slot0<void>      Slot;
  typedef sigc::slot1<void, int> SlotInt;
  typedef sigc::slot0<CurlGet*>  SlotFactory;

  Poll() : m_readSet(new fd_set), m_writeSet(new fd_set), m_exceptSet(new fd_set) {}
  ~Poll() { delete m_readSet; delete m_writeSet; delete m_exceptSet; }

  void        poll(utils::Timer t);

  SlotFactory get_http_factory();

  void        slot_read_stdin(SlotInt s)      { m_slotReadStdin = s; }
  void        slot_select_interrupted(Slot s) { m_slotSelectInterrupted = s; }

private:
  Poll(const Poll&);
  void operator = (const Poll&);

  void        work();
  void        work_input();

  SlotInt     m_slotReadStdin;
  Slot        m_slotSelectInterrupted;

  int         m_maxFd;
  fd_set*     m_readSet;
  fd_set*     m_writeSet;
  fd_set*     m_exceptSet;

  CurlStack   m_curlStack;
};

}

#endif
