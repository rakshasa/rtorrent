#include "config.h"

#include <errno.h>
#include <stdexcept>
#include <sstream>
#include <ncurses.h>
#include <sigc++/bind.h>
#include <torrent/torrent.h>

#include "poll.h"
#include "curl_get.h"

namespace core {

void
Poll::poll(utils::Timer timeout) {
  FD_ZERO(m_readSet);
  FD_ZERO(m_writeSet);
  FD_ZERO(m_exceptSet);

  FD_SET(0, m_readSet);

  torrent::mark(m_readSet, m_writeSet, m_exceptSet, &m_maxFd);

  if (m_curlStack.is_busy()) {
    int n;

    m_curlStack.fdset(m_readSet, m_writeSet, m_exceptSet, &n);
    m_maxFd = std::max(m_maxFd, n);
  }

  timeout = std::min(timeout, utils::Timer(torrent::get(torrent::TIME_SELECT)));
  timeval t = timeout.tval();

  errno = 0;
  m_maxFd = select(m_maxFd + 1, m_readSet, m_writeSet, m_exceptSet, &t);

  if (m_maxFd >= 0) {
    work();

  } else if (errno == EINTR) {
    m_slotSelectInterrupted();
    work_input();

  } else if (errno < 0) {
    throw std::runtime_error("Poll::work(): select error");
  }
}

void
Poll::work() {
  if (FD_ISSET(0, m_readSet))
    work_input();

  if (m_curlStack.is_busy())
    m_curlStack.perform();

  torrent::work(m_readSet, m_writeSet, m_exceptSet, m_maxFd);
}

void
Poll::work_input() {
  int key;

  while ((key = getch()) != ERR)
    m_slotReadStdin(key);
}  

Poll::SlotFactory
Poll::get_http_factory() {
  return sigc::bind(sigc::ptr_fun(&core::CurlGet::new_object), &m_curlStack);
}

}
