#include "config.h"

#include <cerrno>
#include <cstring>
#include <signal.h>
#include <stdexcept>
#include <string>

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

  struct sigaction sa;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sa.sa_handler = &SignalHandler::caught;

  if (sigaction(signum, &sa, NULL) == -1)
    throw std::logic_error("Could not set sigaction: " + std::string(std::strerror(errno)));
  else
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
    throw std::logic_error("Could not set sigaction: " + std::string(std::strerror(errno)));
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
