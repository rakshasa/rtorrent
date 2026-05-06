#ifndef RTORRENT_UTILS_WATCH_READY_QUEUE_H
#define RTORRENT_UTILS_WATCH_READY_QUEUE_H

#include <chrono>
#include <cstdint>
#include <ctime>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <torrent/utils/scheduler.h>

namespace utils {

class WatchReadyQueue {
public:
  WatchReadyQueue();
  ~WatchReadyQueue();

  void                push(const std::string& command, const std::string& path);
  void                shutdown();
  bool                empty() const { return m_entries.empty(); }

private:
  using time_type = std::chrono::microseconds;

  static constexpr auto quiet_time = std::chrono::milliseconds(500);
  static constexpr auto retry_time = std::chrono::milliseconds(250);
  static constexpr auto stale_time = std::chrono::seconds(10);

  struct FileStatus {
    bool     m_exists{};
    bool     m_regular{};
    int64_t  m_size{};
    time_t   m_mtime{};
  };

  struct Entry {
    bool        is_nonempty_regular() const;

    std::string m_command;
    std::string m_path;
    FileStatus  m_status;
    bool        m_has_status{};
    time_type   m_first_seen{};
    time_type   m_last_changed{};
  };

  using ready_list = std::vector<std::pair<std::string, std::string>>;

  static FileStatus  file_status(const std::string& path);
  static bool        same_status(const FileStatus& lhs, const FileStatus& rhs);

  void               process();
  void               update_status(Entry* entry);
  void               schedule();
  time_type          next_time(const Entry& entry) const;

  std::map<std::string, Entry>   m_entries;
  torrent::utils::SchedulerEntry m_task_process;
  bool                           m_active{true};
};

inline bool
WatchReadyQueue::Entry::is_nonempty_regular() const {
  return m_status.m_exists && m_status.m_regular && m_status.m_size > 0;
}

}

#endif
