#include "config.h"

#include <stdexcept>
#include <ncurses.h>

#include "poll.h"

namespace engine {

void
Poll::poll() {
  FD_ZERO(&m_readSet);
  FD_ZERO(&m_writeSet);
  FD_ZERO(&m_exceptSet);

  m_maxFd = 1;

  if (m_readStdin)
    FD_SET(0, &m_readSet);

  timeval timeout = {60, 0};

  m_maxFd = select(m_maxFd, &m_readSet, &m_writeSet, &m_exceptSet, &timeout);

  if (m_maxFd < 0)
    throw std::runtime_error("Poll::work(): select error");
}

void
Poll::work() {
  if (m_readStdin && FD_ISSET(0, &m_readSet)) {
    int key;

    while ((key = getch()) >= 0)
      m_readStdin(key);
  }
}

}
