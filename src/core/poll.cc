#include "config.h"

#include <stdexcept>
#include <sstream>
#include <ncurses.h>
#include <sigc++/bind.h>
#include <torrent/torrent.h>

#include "poll.h"
#include "curl_get.h"

namespace core {

void
Poll::poll() {
  FD_ZERO(&m_readSet);
  FD_ZERO(&m_writeSet);
  FD_ZERO(&m_exceptSet);

  torrent::mark(&m_readSet, &m_writeSet, &m_exceptSet, &m_maxFd);

  m_maxFd = std::max(m_maxFd, 1);
    
  FD_SET(0, &m_readSet);

  if (m_curlStack.is_busy())
    m_curlStack.fdset(&m_readSet, &m_writeSet, &m_exceptSet, &m_maxFd);

  uint64_t t = torrent::get(torrent::TIME_SELECT);

  if (t > 1000000)
    t = 1000000;

  timeval timeout = {t / 1000000, t % 1000000};

  m_maxFd = select(m_maxFd + 1, &m_readSet, &m_writeSet, &m_exceptSet, &timeout);

  if (m_maxFd >= 0)
    work();

  else if (errno == EINTR)
    m_slotSelectInterrupted();

  else if (errno < 0)
    throw std::runtime_error("Poll::work(): select error");
}

void
Poll::work() {
  if (FD_ISSET(0, &m_readSet)) {
    int key;

    while ((key = getch()) != ERR)
      m_slotReadStdin(key);
  }

  if (m_curlStack.is_busy())
    m_curlStack.perform();

  torrent::work(&m_readSet, &m_writeSet, &m_exceptSet, m_maxFd);
}

void
Poll::register_http() {
  torrent::Http::set_factory(sigc::bind(sigc::ptr_fun(&core::CurlGet::new_object), &m_curlStack));
}

}
