#ifndef RTORRENT_SESSION_SESSION_MANAGER_H
#define RTORRENT_SESSION_SESSION_MANAGER_H

#include <condition_variable>
#include <deque>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include <torrent/common.h>

namespace core {
class Download;
}

namespace utils {
class Lockfile;
}

namespace session {

class ThreadSession;

struct SaveRequest {
  core::Download*                    download;
  std::string                        path;
  std::unique_ptr<std::stringstream> torrent_stream;
  std::unique_ptr<std::stringstream> rtorrent_stream;
  std::unique_ptr<std::stringstream> libtorrent_stream;
};

class SessionManager {
public:
  typedef std::unique_ptr<std::stringstream> stream_ptr;

  // TODO: This should depend on max open sockets / be configurable.

  // TODO: max_concurrent_requests should be checked before something, review.

  constexpr static int max_concurrent_requests = 16;
  constexpr static int max_concurrent_saves    = 16;
  constexpr static int max_cleanup_saves       = 64;

  SessionManager(torrent::utils::Thread* thread);
  ~SessionManager();

  bool                is_used() const;

  std::string         path() const;
  void                set_path(const std::string& path);

  bool                use_fsyncdisk() const;
  void                set_use_fsyncdisk(bool use_fsyncdisk);

  bool                use_lock() const;
  void                set_use_lock(bool use_lock);

  void                save_full_download(core::Download* download);
  void                save_resume_download(core::Download* download);
  void                remove_download(core::Download* download);

protected:
  friend class Control;
  friend class ThreadSession;

  void                save_download(core::Download* download, bool skip_static);

  void                start();
  void                cleanup();

private:
  void                process_pending_resume_builds();
  void                process_save_request();
  void                process_save_request_with_pending_callback();
  void                process_next_save_request_unsafe();
  void                process_finished_saves();

  void                wait_for_one_save_unsafe(std::unique_lock<std::mutex>& lock);

  // Requires a higher number of open sockets, and should only be used during shutdown.
  void                flush_all_and_wait_unsafe(std::unique_lock<std::mutex>& lock);

  bool                remove_or_replace_unsafe(SaveRequest& download);
  bool                remove_completely_unsafe(core::Download* download, std::unique_lock<std::mutex>& lock);

  torrent::utils::Thread* m_thread;

  bool                m_freeze_info{};
  std::string         m_path;
  bool                m_use_fsyncdisk{true};
  bool                m_use_lock{true};

  std::mutex          m_mutex;
  bool                m_active{};

  typedef std::pair<std::future<void>, SaveRequest> ProcessingSave;

  std::deque<SaveRequest>     m_save_requests;
  std::list<ProcessingSave>   m_processing_saves;
  std::condition_variable     m_finished_condition;
  std::vector<ProcessingSave> m_finished_saves;

  std::unique_ptr<utils::Lockfile> m_lockfile;

  // Pending builds are only ever locked by main thread.
  std::mutex                  m_pending_builds_mutex;
  std::deque<core::Download*> m_pending_builds;
};

inline bool        SessionManager::is_used() const       { return !m_path.empty(); }
inline std::string SessionManager::path() const          { return m_path; }
inline bool        SessionManager::use_fsyncdisk() const { return true; }
inline bool        SessionManager::use_lock() const      { return m_use_lock; }

} // namespace session

#endif // RTORRENT_SESSION_SESSION_MANAGER_H
