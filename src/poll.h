#ifndef RTORRENT_POLL_H
#define RTORRENT_POLL_H

#include <sys/select.h>
#include <sigc++/slot.h>

class Poll {
public:
  typedef sigc::slot1<void, int> SlotInt;

  Poll() : m_running(true) {}

  bool      is_running()            { return m_running; }

  void      poll();
  void      work();

  void      slot_read_stdin(SlotInt s) { m_readStdin = s; }

private:
  bool      m_running;
  SlotInt   m_readStdin;

  int       m_maxFd;
  fd_set    m_readSet;
  fd_set    m_writeSet;
  fd_set    m_exceptSet;
};

#endif
