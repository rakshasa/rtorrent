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
  void                clear();
  void                disable();
  bool                empty() const { return m_entries.empty(); }

private:
  using time_type = std::chrono::microseconds;

  struct FileStatus {
    bool     m_exists{};
    bool     m_regular{};
    int64_t  m_size{};
    time_t   m_mtime{};
  };

  struct Entry {
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
  void               update_status(Entry* entry, time_type now);
  void               schedule();
  time_type          next_time(const Entry& entry, time_type now) const;

  std::map<std::string, Entry>   m_entries;
  torrent::utils::SchedulerEntry m_task_process;
  bool                           m_disabled{};
};

}

#endif
