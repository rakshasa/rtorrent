#ifndef RTORRENT_SIGNAL_HANDLER_H
#define RTORRENT_SIGNAL_HANDLER_H

#include <signal.h>
#include <sigc++/slot.h>

class SignalHandler {
public:
  typedef sigc::slot0<void> Slot;

  static const unsigned int HIGHEST_SIGNAL = 32;
  
  static void set_default(unsigned int signum);
  static void set_ignore(unsigned int signum);
  static void set_handler(unsigned int signum, Slot slot);

private:
  static void caught(int signum);

  static Slot m_handlers[HIGHEST_SIGNAL];
};

#endif
