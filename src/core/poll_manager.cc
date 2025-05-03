#include "config.h"

#include "core/poll_manager.h"

#include <cerrno>
#include <unistd.h>
#include <torrent/exceptions.h>
#include <torrent/poll.h>

namespace core {

// TODO: No need for this function.

torrent::Poll*
create_poll() {
  int max_open_sockets = sysconf(_SC_OPEN_MAX);

  torrent::Poll* poll = torrent::Poll::create(max_open_sockets);

  if (poll == nullptr)
    throw torrent::internal_error("Unable to create poll object: " + std::string(std::strerror(errno)));

  return poll;
}

}
