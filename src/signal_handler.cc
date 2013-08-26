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

#include <signal.h>
#include <stdexcept>
#include "rak/error_number.h"
#include "signal_handler.h"

#ifdef __sun__
#include <iso/signal_iso.h>
//extern "C" void (*signal (int sig, void (*disp)(int)))(int);
#endif

SignalHandler::slot_void SignalHandler::m_handlers[HIGHEST_SIGNAL];

void
SignalHandler::set_default(unsigned int signum) {
  if (signum > HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::set_default(...) received invalid signal value.");

  signal(signum, SIG_DFL);
  m_handlers[signum] = slot_void();
}

void
SignalHandler::set_ignore(unsigned int signum) {
  if (signum > HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::set_ignore(...) received invalid signal value.");

  signal(signum, SIG_IGN);
  m_handlers[signum] = slot_void();
}

void
SignalHandler::set_handler(unsigned int signum, slot_void slot) {
  if (signum > HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::set_handler(...) received invalid signal value.");

  if (!slot)
    throw std::logic_error("SignalHandler::set_handler(...) received an empty slot.");

  signal(signum, &SignalHandler::caught);
  m_handlers[signum] = slot;
}

void
SignalHandler::set_sigaction_handler(unsigned int signum, handler_slot slot) {
  if (signum > HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::set_handler(...) received invalid signal value.");

  struct sigaction sa;
  sa.sa_sigaction = slot;
  sa.sa_flags = SA_SIGINFO;

  sigemptyset(&sa.sa_mask);

  if (sigaction(signum, &sa, NULL) == -1)
    throw std::logic_error("Could not set sigaction: " + std::string(rak::error_number::current().c_str()));
}

void
SignalHandler::caught(int signum) {
  if ((unsigned)signum > HIGHEST_SIGNAL)
    throw std::logic_error("SignalHandler::caught(...) received invalid signal from the kernel, bork bork bork.");

  if (!m_handlers[signum])
    throw std::logic_error("SignalHandler::caught(...) received a signal we don't have a handler for.");

  m_handlers[signum]();
}

const char*
SignalHandler::as_string(unsigned int signum) {
  switch (signum) {
  case SIGHUP:
    return "Hangup detected";
  case SIGINT:
    return "Interrupt from keyboard";
  case SIGQUIT:
    return "Quit signal";
  case SIGILL:
    return "Illegal instruction";
  case SIGABRT:
    return "Abort signal";
  case SIGFPE:
    return "Floating point exception";
  case SIGKILL:
    return "Kill signal";
  case SIGSEGV:
    return "Segmentation fault";
  case SIGPIPE:
    return "Broken pipe";
  case SIGALRM:
    return "Timer signal";
  case SIGTERM:
    return "Termination signal";
  case SIGBUS:
    return "Bus error";
  default:
    return "Unlisted";
  }
}
