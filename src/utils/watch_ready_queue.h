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

  using ready_list = std::vector<std::pair<std::string, std::string>>;

  struct Entry {
    std::string command;
    std::string path;
    bool        regular{};
    int64_t     size{-1};
    time_t      mtime{};
    time_type   first_seen{};
    time_type   last_changed{};
    time_type   next_time{};
  };

  void               process();
  void               push_entry(Entry* entry);
  void               update_entry(Entry* entry);
  void               update_next_time(Entry* entry);
  void               update_status(Entry* entry);
  void               schedule();

  std::map<std::string, Entry>   m_entries;
  std::vector<Entry*>            m_entry_queue;
  torrent::utils::SchedulerEntry m_task_process;
  bool                           m_active{true};
};

}

#endif
