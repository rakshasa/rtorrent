#include "config.h"

#include <cassert>
#include <unistd.h>
#include <sys/un.h>
#include <torrent/connection_manager.h>
#include <torrent/torrent.h>
#include <torrent/exceptions.h>
#include <torrent/net/fd.h>
#include <torrent/net/poll.h>
#include <torrent/net/socket_address.h>
#include <torrent/runtime/socket_manager.h>

#include "control.h"
#include "globals.h"
#include "rpc/scgi_task.h"

// TODO: Figure out why moving this to the top causes a build error.
#include "rpc/scgi.h"

namespace rpc {

SCgi::~SCgi() {
  assert(!is_open() && "SCgi::~SCgi() called while open");
}

void
SCgi::open_port(sockaddr* sa, unsigned int length, bool dont_route) {
  torrent::runtime::socket_manager()->open_event_or_throw(this, [&]() {
      int fd = torrent::fd_open_family(torrent::fd_flag_stream | torrent::fd_flag_nonblock | torrent::fd_flag_reuse_address,
                                       sa->sa_family);

      if (fd == -1)
        throw torrent::resource_error("Could not open socket for listening: " + std::string(std::strerror(errno)));

      if (dont_route && !torrent::fd_set_dont_route(fd, true)) {
        torrent::fd_close(fd);
        throw torrent::resource_error("Could not set socket option IP_DONTROUTE: " + std::string(std::strerror(errno)));
      }

      set_file_descriptor(fd);

      open(reinterpret_cast<sockaddr*>(sa), length);
    });

  torrent::connection_manager()->inc_socket_count();
}

void
SCgi::open_named(const std::string& filename) {
  if (filename.empty() || filename.size() > 4096)
    throw torrent::resource_error("Invalid filename length.");

  auto buffer = std::make_unique<char[]>(sizeof(sockaddr_un) + filename.size() + 1);

  sockaddr_un* sa = reinterpret_cast<sockaddr_un*>(buffer.get());
  sa->sun_family = AF_LOCAL;
  std::memcpy(sa->sun_path, filename.c_str(), filename.size() + 1);

  torrent::runtime::socket_manager()->open_event_or_throw(this, [&]() {
      int fd = torrent::fd_open_local(torrent::fd_flag_stream | torrent::fd_flag_nonblock | torrent::fd_flag_reuse_address);

      if (fd == -1)
        throw torrent::resource_error("Could not open socket for listening: " + std::string(std::strerror(errno)));

      set_file_descriptor(fd);

      open(reinterpret_cast<sockaddr*>(sa), offsetof(struct sockaddr_un, sun_path) + filename.size() + 1);
    });

  torrent::connection_manager()->inc_socket_count();

  m_path = filename;
}

void
SCgi::open(sockaddr* sa, unsigned int length) {
  try {
    if (::bind(file_descriptor(), sa, length) == -1)
      throw torrent::resource_error("Could not bind socket for listening: " + std::string(std::strerror(errno)));

    if (!torrent::fd_listen(file_descriptor(), max_tasks))
      throw torrent::resource_error("Could not prepare socket for listening: " + std::string(std::strerror(errno)));

  } catch (torrent::resource_error& e) {
    torrent::fd_close(file_descriptor());
    set_file_descriptor(-1);
    throw;
  }
}

void
SCgi::activate() {
  assert(torrent::this_thread::thread() == scgi_thread::thread());

  torrent::this_thread::poll()->open(this);
  torrent::this_thread::poll()->insert_read(this);
  torrent::this_thread::poll()->insert_error(this);
}

// TODO: This should close the fd to avoid reuse.
void
SCgi::stop() {
  assert(torrent::this_thread::thread() == scgi_thread::thread());

  if (!is_open())
    return;

  for (SCgiTask* itr = m_task, *last = m_task + max_tasks; itr != last; ++itr)
    if (itr->is_open())
      itr->close();

  torrent::runtime::socket_manager()->close_event_or_throw(this, [this]() {
      torrent::this_thread::poll()->remove_and_close(this);

      torrent::fd_close(file_descriptor());
      set_file_descriptor(-1);
    });

  torrent::connection_manager()->dec_socket_count();

  if (!m_path.empty())
    ::unlink(m_path.c_str());
}

void
SCgi::event_read() {
  while (true) {
    auto* task = std::find_if(m_task, m_task + max_tasks, std::mem_fn(&SCgiTask::is_available));

    if (task == m_task + max_tasks) {
      // TODO: Currently just close, although we should remove ourselves from read.
      int fd = torrent::fd_accept(file_descriptor());

      if (fd != -1)
        torrent::fd_close(fd);

      continue;
    }

    auto open_func = [this, task]() {
        int fd = torrent::fd_accept(file_descriptor());

        if (fd == -1) {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

          throw torrent::resource_error("Listener port accept() failed: " + std::string(std::strerror(errno)));
        }

        task->open(this, fd);
      };

    auto cleanup_func = [task]() {
        task->cancel_open();
      };

    bool result = torrent::runtime::socket_manager()->open_event_or_cleanup(task, open_func, cleanup_func);

    if (!result)
      break;
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
