#ifndef RTORRENT_RPC_EXEC_FILE_H
#define RTORRENT_RPC_EXEC_FILE_H

#include <torrent/object.h>

#include "utils/waitpid_queue.h"

namespace rpc {

class ExecFile {
public:
  static constexpr unsigned int max_args    = 128;
  // Room for the arguments that are not strings already (values, lists, a command's result), printed one after
  // the other: Linux's own limit for a single argument (MAX_ARG_STRLEN, 128 KiB). 4096 bytes overflowed on a
  // torrent with a couple of hundred trackers, whose URLs ruTorrent's History plugin passes as one argument, and
  // the exception ended the whole event handler chain (event.download.erased skipped ~_delete_tied).
  static constexpr unsigned int buffer_size = 128 * 1024;

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

  utils::WaitpidQueue m_waitpid_queue;
};

}

#endif
