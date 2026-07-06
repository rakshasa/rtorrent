#ifndef RTORRENT_SIGNAL_HANDLER_H
#define RTORRENT_SIGNAL_HANDLER_H

#include <functional>
#include <signal.h>

class SignalHandler {
public:
  typedef std::function<void ()> slot_void;

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

  static void         set_block(unsigned int signum);
  static void         set_unblock(unsigned int signum);

  static void         set_sigchild_ignore();

  static void         set_sigaction_handler(unsigned int signum, handler_slot slot);

  static const char*  as_string(unsigned int signum);

private:
  static void         caught(int signum);

  static slot_void    m_handlers[HIGHEST_SIGNAL];
};

#endif
