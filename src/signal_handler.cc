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
