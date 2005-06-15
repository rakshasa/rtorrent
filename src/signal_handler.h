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

#ifndef RTORRENT_SIGNAL_HANDLER_H
#define RTORRENT_SIGNAL_HANDLER_H

#include <signal.h>
#include <sigc++/slot.h>

class SignalHandler {
public:
  typedef sigc::slot0<void> Slot;

  static const unsigned int HIGHEST_SIGNAL = 32;
  
  static void         set_default(unsigned int signum);
  static void         set_ignore(unsigned int signum);
  static void         set_handler(unsigned int signum, Slot slot);

  static const char*  as_string(unsigned int signum);

private:
  static void         caught(int signum);

  static Slot m_handlers[HIGHEST_SIGNAL];
};

#endif
