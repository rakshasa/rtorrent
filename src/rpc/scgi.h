#ifndef RTORRENT_RPC_SCGI_H
#define RTORRENT_RPC_SCGI_H

#include <functional>
#include <torrent/event.h>

#include "rpc/scgi_task.h"

namespace rpc {

class SCgi : public torrent::Event {
public:
  static const int max_tasks = 100;

  ~SCgi() override;

  const char*         type_name() const override { return "scgi"; }

  void                open_port(sockaddr* sa, unsigned int length, bool dont_route);
  void                open_named(const std::string& filename);

  void                activate();

  void                stop();

  const std::string&  path() const { return m_path; }

  int                 log_fd() const     { return m_logFd; }
  void                set_log_fd(int fd) { m_logFd = fd; }

  void                event_read() override;
  void                event_write() override;
  void                event_error() override;

private:
  void                open(sockaddr* sa, unsigned int length);

  std::string         m_path;
  int                 m_logFd{-1};
  SCgiTask            m_task[max_tasks];
};

}

#endif
