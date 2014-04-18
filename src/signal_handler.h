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

#ifndef RTORRENT_SIGNAL_HANDLER_H
#define RTORRENT_SIGNAL_HANDLER_H

#include <signal.h>
#include <tr1/functional>

class SignalHandler {
public:
  typedef std::tr1::function<void ()> slot_void;

  // typedef void (*handler_slot)(int, siginfo_t *info, ucontext_t *uap);
  typedef void (*handler_slot)(int, siginfo_t*, void*);

#ifdef NSIG
  static const unsigned int HIGHEST_SIGNAL = NSIG;
#else
  // Let's be on the safe side.
  static const unsigned int HIGHEST_SIGNAL = 32;
#endif

  static void         set_default(unsigned int signum);
  static void         set_ignore(unsigned int signum);
  static void         set_handler(unsigned int signum, slot_void slot);

  static void         set_sigaction_handler(unsigned int signum, handler_slot slot);

  static const char*  as_string(unsigned int signum);

private:
  static void         caught(int signum);

  static slot_void    m_handlers[HIGHEST_SIGNAL];
};

#endif
