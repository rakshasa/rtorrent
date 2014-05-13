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

#ifndef RTORRENT_INPUT_INPUT_EVENT_H
#define RTORRENT_INPUT_INPUT_EVENT_H

#include <torrent/event.h>
#include <torrent/poll.h>
#include lt_tr1_functional

namespace input {

class InputEvent : public torrent::Event {
public:
  typedef std::function<void (int)> slot_int;

  InputEvent(int fd) { m_fileDesc = fd; }

  void                insert(torrent::Poll* p);
  void                remove(torrent::Poll* p);

  void                event_read();
  void                event_write();
  void                event_error();

  void                slot_pressed(slot_int s) { m_slotPressed = s; }

private:
  slot_int            m_slotPressed;
};

}

#endif
