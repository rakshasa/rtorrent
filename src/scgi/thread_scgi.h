#ifndef RTORRENT_SCGI_THREAD_SCGI_H
#define RTORRENT_SCGI_THREAD_SCGI_H

#include <atomic>
#include <string>
#include <torrent/utils/thread.h>

namespace rpc {
class SCgi;
}

namespace scgi {

class ThreadScgiInternal;

class ThreadScgi : public torrent::utils::Thread {
public:

  static void         create_thread();
  static void         destroy_thread();
  static ThreadScgi*  thread_scgi();

  const char*         name() const override  { return "rtorrent-scgi"; }

  rpc::SCgi*          scgi();
  bool                set_scgi(rpc::SCgi* scgi);

  void                set_rpc_log(const std::string& filename);

protected:
  friend class ThreadScgiInternal;

  ThreadScgi() = default;

  static auto         internal_thread_scgi() { return m_thread_scgi; }

  void                cleanup_thread() override;

  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;

private:
  void                task_touch_log();
  void                change_rpc_log();

  static ThreadScgi*  m_thread_scgi;

  std::atomic<rpc::SCgi*> m_scgi{nullptr};
  std::string             m_rpc_log_filename;
};

} // namespace scgi

#endif // RTORRENT_SCGI_THREAD_SCGI_H
