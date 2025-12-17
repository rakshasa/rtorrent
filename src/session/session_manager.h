#ifndef RTORRENT_SESSION_SESSION_MANAGER_H
#define RTORRENT_SESSION_SESSION_MANAGER_H

#include <deque>
#include <memory>
#include <sstream>
#include <string>
#include <torrent/common.h>

namespace core {
class Download;
} // namespace core

namespace utils {
class Lockfile;
} // namespace utils

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

  SessionManager(torrent::utils::Thread* thread);
  ~SessionManager();

  bool                is_used() const;

  // TODO: Replace with protected `bool shutdown_if_done()`.
  bool                is_empty();
  // bool                is_empty_and_done();

  void                start();

  std::string         path() const;
  void                set_path(const std::string& path);

  bool                use_lock() const;
  void                set_use_lock(bool use_lock);

  void                freeze_info();

  void                save_download(core::Download* download, std::string path, stream_ptr torrent_stream, stream_ptr rtorrent_stream, stream_ptr libtorrent_stream);
  void                remove_download(core::Download* download, std::string path);

protected:
  friend class Control;
  friend class ThreadSession;

  void                cleanup();

private:
  void                process_save_request();

  void                save_download_unsafe(const SaveRequest& request);
  bool                save_download_stream_unsafe(const std::string& path, const std::unique_ptr<std::stringstream>& stream);

  torrent::utils::Thread* m_thread;

  bool                m_freeze_info{};
  std::string         m_path;
  bool                m_use_lock{true};

  std::mutex          m_mutex;
  bool                m_active{};

  std::deque<SaveRequest>          m_save_requests;
  std::unique_ptr<utils::Lockfile> m_lockfile;
};

inline bool        SessionManager::is_used() const  { return !m_path.empty(); }
inline std::string SessionManager::path() const     { return m_path; }
inline bool        SessionManager::use_lock() const { return m_use_lock; }

} // namespace session

#endif // RTORRENT_SESSION_SESSION_MANAGER_H
