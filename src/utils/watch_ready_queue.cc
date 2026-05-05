#include "config.h"

#include "utils/watch_ready_queue.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <torrent/utils/file_stat.h>

#include "control.h"
#include "core/manager.h"
#include "globals.h"
#include "rpc/rpc_manager.h"

namespace utils {

namespace {

constexpr auto watch_ready_quiet_time = std::chrono::milliseconds(500);
constexpr auto watch_ready_retry_time = std::chrono::milliseconds(250);
constexpr auto watch_ready_stale_time = std::chrono::seconds(10);

void
log_dropped_unready_file(const std::string& path) {
  if (control != nullptr && control->core() != nullptr)
    control->core()->push_log_std("Dropped unready watch file after timeout: \"" + path + "\"");
}

} // namespace

WatchReadyQueue::WatchReadyQueue() {
  m_task_process.slot() = std::bind(&WatchReadyQueue::process, this);
}

WatchReadyQueue::~WatchReadyQueue() {
  torrent::this_thread::scheduler()->erase(&m_task_process);
}

void
WatchReadyQueue::push(const std::string& command, const std::string& path) {
  if (m_disabled)
    return;

  auto now = torrent::this_thread::cached_time();
  auto result = m_entries.emplace(path, Entry());
  auto& entry = result.first->second;

  if (result.second) {
    entry.m_path = path;
    entry.m_first_seen = now;
  }

  entry.m_command = command;
  entry.m_last_changed = now;
  update_status(&entry, now);
  schedule();
}

void
WatchReadyQueue::clear() {
  m_entries.clear();
  torrent::this_thread::scheduler()->erase(&m_task_process);
}

void
WatchReadyQueue::disable() {
  m_disabled = true;
  clear();
}

WatchReadyQueue::FileStatus
WatchReadyQueue::file_status(const std::string& path) {
  torrent::utils::FileStat fs;
  FileStatus result;

  if (!fs.update(expand_path(path)))
    return result;

  result.m_exists = true;
  result.m_regular = fs.is_regular();
  result.m_size = fs.size();
  result.m_mtime = fs.modified_time();

  return result;
}

bool
WatchReadyQueue::same_status(const FileStatus& lhs, const FileStatus& rhs) {
  return lhs.m_exists == rhs.m_exists &&
         lhs.m_regular == rhs.m_regular &&
         lhs.m_size == rhs.m_size &&
         lhs.m_mtime == rhs.m_mtime;
}

void
WatchReadyQueue::update_status(Entry* entry, time_type now) {
  FileStatus status = file_status(entry->m_path);

  if (!entry->m_has_status || !same_status(entry->m_status, status)) {
    entry->m_status = status;
    entry->m_has_status = true;
    entry->m_last_changed = now;
  }
}

void
WatchReadyQueue::process() {
  if (m_disabled || (control != nullptr && control->is_shutdown_started())) {
    clear();
    return;
  }

  auto now = torrent::this_thread::cached_time();
  ready_list ready;

  for (auto itr = m_entries.begin(); itr != m_entries.end(); ) {
    Entry& entry = itr->second;
    update_status(&entry, now);

    bool nonempty_regular = entry.m_status.m_exists &&
                            entry.m_status.m_regular &&
                            entry.m_status.m_size > 0;
    bool stable = now - entry.m_last_changed >= watch_ready_quiet_time;
    bool stale = now - entry.m_first_seen >= watch_ready_stale_time;

    if (nonempty_regular && stable) {
      ready.emplace_back(entry.m_command, entry.m_path);
      itr = m_entries.erase(itr);
      continue;
    }

    if (!nonempty_regular && stale) {
      log_dropped_unready_file(entry.m_path);
      itr = m_entries.erase(itr);
      continue;
    }

    ++itr;
  }

  schedule();

  for (const auto& item : ready)
    rpc::commands.call_catch(item.first.c_str(), rpc::make_target(), item.second);
}

void
WatchReadyQueue::schedule() {
  if (m_entries.empty()) {
    torrent::this_thread::scheduler()->erase(&m_task_process);
    return;
  }

  auto now = torrent::this_thread::cached_time();
  auto earliest = next_time(m_entries.begin()->second, now);

  for (auto itr = std::next(m_entries.begin()); itr != m_entries.end(); ++itr)
    earliest = std::min(earliest, next_time(itr->second, now));

  if (m_task_process.is_scheduled())
    torrent::this_thread::scheduler()->update_wait_until(&m_task_process, earliest);
  else
    torrent::this_thread::scheduler()->wait_until(&m_task_process, earliest);
}

WatchReadyQueue::time_type
WatchReadyQueue::next_time(const Entry& entry, time_type now) const {
  if (entry.m_status.m_exists &&
      entry.m_status.m_regular &&
      entry.m_status.m_size > 0)
    return entry.m_last_changed + watch_ready_quiet_time;

  return std::min(now + watch_ready_retry_time,
                  entry.m_first_seen + watch_ready_stale_time);
}

}
