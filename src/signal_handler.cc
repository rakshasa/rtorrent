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

#include "config.h"

#include <stdexcept>
#include "signal_handler.h"

SignalHandler::Slot SignalHandler::m_handlers[HIGHEST_SIGNAL];

void
SignalHandler::set_default(unsigned int signum) {
  if (signum >= HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::clear(...) received invalid signal value");

  signal(signum, SIG_DFL);
  m_handlers[signum].disconnect();
}

void
SignalHandler::set_ignore(unsigned int signum) {
  if (signum >= HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::clear(...) received invalid signal value");

  signal(signum, SIG_IGN);
  m_handlers[signum].disconnect();
}

void
SignalHandler::set_handler(unsigned int signum, Slot slot) {
  if (signum >= HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::handle(...) received invalid signal value");

  if (slot.empty())
    throw std::logic_error("SignalHandler::handle(...) received an empty slot");

  signal(signum, &SignalHandler::caught);
  m_handlers[signum] = slot;
}

void
SignalHandler::caught(int signum) {
  if ((unsigned)signum >= HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::caught(...) received invalid signal from the kernel, bork bork bork");

  if (m_handlers[signum].empty())
    throw std::logic_error("SignalHandler::caught(...) received a signal we don't have a handler for");

  m_handlers[signum]();
}
