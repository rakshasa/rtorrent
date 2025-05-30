#ifndef RTORRENT_RPC_EXEC_FILE_H
#define RTORRENT_RPC_EXEC_FILE_H

#include <torrent/object.h>

namespace rpc {

class ExecFile {
public:
  static constexpr unsigned int max_args    = 128;
  static constexpr unsigned int buffer_size = 4096;

  static constexpr int flag_expand_tilde = 0x1;
  static constexpr int flag_throw        = 0x2;
  static constexpr int flag_capture      = 0x4;
  static constexpr int flag_background   = 0x8;

  int                 log_fd() const     { return m_log_fd; }
  void                set_log_fd(int fd) { m_log_fd = fd; }

  int                 execute(const char* file, char* const* argv, int flags);
  torrent::Object     execute_object(const torrent::Object& rawArgs, int flags);

private:
  int                 m_log_fd{-1};
  std::string         m_capture;
};

}

#endif
