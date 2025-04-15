#include "config.h"

#include <rak/error_number.h>
#include <rak/socket_address.h>
#include <sys/un.h>
#include <torrent/connection_manager.h>
#include <torrent/poll.h>
#include <torrent/torrent.h>
#include <torrent/exceptions.h>

#include "control.h"
#include "globals.h"
#include "rpc/scgi_task.h"
#include "utils/socket_fd.h"

#include "rpc/scgi.h"

namespace rpc {

SCgi::~SCgi() {
  if (!get_fd().is_valid())
    return;

  for (SCgiTask* itr = m_task, *last = m_task + max_tasks; itr != last; ++itr)
    if (itr->is_open())
      itr->close();

  deactivate();
  torrent::connection_manager()->dec_socket_count();

  get_fd().close();
  get_fd().clear();

  if (!m_path.empty())
    ::unlink(m_path.c_str());
}

void
SCgi::open_port(void* sa, unsigned int length, bool dontRoute) {
  if (!get_fd().open_stream() ||
      (dontRoute && !get_fd().set_dont_route(true)))
    throw torrent::resource_error("Could not open socket for listening: " + std::string(rak::error_number::current().c_str()));

  open(sa, length);
}

void
SCgi::open_named(const std::string& filename) {
  if (filename.empty() || filename.size() > 4096)
    throw torrent::resource_error("Invalid filename length.");

  char buffer[sizeof(sockaddr_un) + filename.size()];
  sockaddr_un* sa = reinterpret_cast<sockaddr_un*>(buffer);

#ifdef __sun__
  sa->sun_family = AF_UNIX;
#else
  sa->sun_family = AF_LOCAL;
#endif

  std::memcpy(sa->sun_path, filename.c_str(), filename.size() + 1);

  if (!get_fd().open_local())
    throw torrent::resource_error("Could not open socket for listening.");

  open(sa, offsetof(struct sockaddr_un, sun_path) + filename.size() + 1);
  m_path = filename;
}

void
SCgi::open(void* sa, unsigned int length) {
  try {
    if (!get_fd().set_nonblock() ||
        !get_fd().set_reuse_address(true) ||
        !get_fd().bind(*reinterpret_cast<rak::socket_address*>(sa), length) ||
        !get_fd().listen(max_tasks))
      throw torrent::resource_error("Could not prepare socket for listening: " + std::string(rak::error_number::current().c_str()));

    torrent::connection_manager()->inc_socket_count();

  } catch (torrent::resource_error& e) {
    get_fd().close();
    get_fd().clear();

    throw e;
  }
}

// TODO: Verify this is run in correct thread, also only ever call poll methods from thread_self.

void
SCgi::activate() {
  worker_thread->poll()->open(this);
  worker_thread->poll()->insert_read(this);
  worker_thread->poll()->insert_error(this);
}

void
SCgi::deactivate() {
  worker_thread->poll()->remove_read(this);
  worker_thread->poll()->remove_error(this);
  worker_thread->poll()->close(this);
}

void
SCgi::event_read() {
  rak::socket_address sa;
  utils::SocketFd fd;

  while ((fd = get_fd().accept(&sa)).is_valid()) {
    SCgiTask* task = std::find_if(m_task, m_task + max_tasks, std::mem_fn(&SCgiTask::is_available));

    if (task == m_task + max_tasks) {
      fd.close();
      continue;
    }

    task->open(this, fd.get_fd());
  }
}

void
SCgi::event_write() {
  throw torrent::internal_error("Listener does not support write().");
}

void
SCgi::event_error() {
  throw torrent::internal_error("SCGI listener port received an error event.");
}

}
