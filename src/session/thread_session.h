#ifndef RTORRENT_SESSION_THREAD_SESSION_H
#define RTORRENT_SESSION_THREAD_SESSION_H

#include <torrent/utils/thread.h>

namespace session {

class SessionManager;
class ThreadSessionInternal;

class ThreadSession : public torrent::utils::Thread {
public:

  static void           create_thread();
  static void           destroy_thread();
  static ThreadSession* thread_session();

  const char*         name() const override     { return "rtorrent-session"; }

  SessionManager*     manager() const           { return m_manager.get(); }

protected:
  friend class ThreadSessionInternal;

  ThreadSession() = default;

  static auto         internal_thread_session() { return m_thread_session; }

  void                init_thread_pre_start() override;
  void                cleanup_thread() override;

  void                      call_events() override;
  std::chrono::microseconds next_timeout() override;

private:
  static ThreadSession*   m_thread_session;

  std::unique_ptr<SessionManager> m_manager;
};

} // namespace session

#endif // RTORRENT_SESSION_THREAD_SESSION_H
