#ifndef RTORRENT_CORE_POLL_H
#define RTORRENT_CORE_POLL_H

#include <sys/select.h>
#include <sigc++/slot.h>

#include "curl_stack.h"

namespace core {

class Poll {
public:
  typedef sigc::slot0<void> Slot;
  typedef sigc::slot1<void, int> SlotInt;

  void      poll();

  void      register_http();

  void      slot_read_stdin(SlotInt s)      { m_slotReadStdin = s; }
  void      slot_select_interrupted(Slot s) { m_slotSelectInterrupted = s; }

private:
  void      work();
  void      work_input();

  SlotInt   m_slotReadStdin;
  Slot      m_slotSelectInterrupted;

  int       m_maxFd;
  fd_set    m_readSet;
  fd_set    m_writeSet;
  fd_set    m_exceptSet;

  CurlStack m_curlStack;
};

}

#endif
