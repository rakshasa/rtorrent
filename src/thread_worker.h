#ifndef RTORRENT_THREAD_WORKER_H
#define RTORRENT_THREAD_WORKER_H

#include <atomic>
#include <string>
#include <torrent/utils/thread.h>

namespace rpc {
class SCgi;
}

class ThreadWorker : public torrent::utils::Thread {
public:
  ThreadWorker() = default;
  ~ThreadWorker();

  const char*         name() const override      { return "rtorrent scgi"; }

  void                init_thread() override;
  void                cleanup_thread() override;

  rpc::SCgi*          scgi()                     { return m_scgi; }
  bool                set_scgi(rpc::SCgi* scgi);

  void                set_rpc_log(const std::string& filename);

private:
  void                task_touch_log();
  void                change_rpc_log();

  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;

  std::atomic<rpc::SCgi*>   m_scgi{nullptr};
  std::string               m_rpc_log_filename;
};

#endif
