#ifndef RTORRENT_RPC_SCGI_H
#define RTORRENT_RPC_SCGI_H

#include <functional>
#include <torrent/event.h>

#include "rpc/scgi_task.h"
#include "utils/socket_fd.h"

namespace utils {
  class SocketFd;
}

namespace rpc {

class SCgi : public torrent::Event {
public:
  static const int max_tasks = 100;

  ~SCgi() override;

  const char*         type_name() const override { return "scgi"; }

  void                open_port(void* sa, unsigned int length, bool dontRoute);
  void                open_named(const std::string& filename);

  void                activate();
  void                deactivate();

  const std::string&  path() const { return m_path; }

  int                 log_fd() const     { return m_logFd; }
  void                set_log_fd(int fd) { m_logFd = fd; }

  void                event_read() override;
  void                event_write() override;
  void                event_error() override;

  utils::SocketFd&    get_fd()            { return *reinterpret_cast<utils::SocketFd*>(&m_fileDesc); }

private:
  void                open(void* sa, unsigned int length);

  std::string         m_path;
  int                 m_logFd{-1};
  SCgiTask            m_task[max_tasks];
};

}

#endif
