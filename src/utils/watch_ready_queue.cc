#include "config.h"

#include "utils/watch_ready_queue.h"

#include <algorithm>
#include <iterator>
#include <torrent/utils/file_stat.h>
#include <torrent/utils/log.h>

#include "globals.h"
#include "rpc/rpc_manager.h"

namespace utils {

WatchReadyQueue::WatchReadyQueue() {
  m_task_process.slot() = [this]() { process(); };
}

WatchReadyQueue::~WatchReadyQueue() {
  torrent::this_thread::scheduler()->erase(&m_task_process);
}

void
WatchReadyQueue::push(const std::string& command, const std::string& path) {
  if (!m_active)
    return;

  auto result = m_entries.emplace(path, Entry());
  auto& entry = result.first->second;

  if (result.second) {
    entry.m_path = path;
    entry.m_first_seen = torrent::this_thread::cached_time();
  }

  entry.m_command = command;
  entry.m_last_changed = torrent::this_thread::cached_time();
  update_status(&entry);
  schedule();
}

void
WatchReadyQueue::shutdown() {
  m_active = false;
  m_entries.clear();
  torrent::this_thread::scheduler()->erase(&m_task_process);
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
WatchReadyQueue::update_status(Entry* entry) {
  FileStatus status = file_status(entry->m_path);

  if (!entry->m_has_status || !same_status(entry->m_status, status)) {
    entry->m_status = status;
    entry->m_has_status = true;
    entry->m_last_changed = torrent::this_thread::cached_time();
  }
}

void
WatchReadyQueue::process() {
  ready_list ready;

  for (auto itr = m_entries.begin(); itr != m_entries.end(); ) {
    Entry& entry = itr->second;
    update_status(&entry);

    bool unchanged_entry = torrent::this_thread::cached_time() - entry.m_last_changed >= quiet_time;
    bool stale_entry = torrent::this_thread::cached_time() - entry.m_first_seen >= stale_time;

    if (entry.is_nonempty_regular() && unchanged_entry) {
      ready.emplace_back(entry.m_command, entry.m_path);
      itr = m_entries.erase(itr);
      continue;
    }

    if (!entry.is_nonempty_regular() && stale_entry) {
      lt_log_print(torrent::LOG_NOTICE, "Dropped unready watch file after timeout: \"%s\"", entry.m_path.c_str());
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

  auto earliest = next_time(m_entries.begin()->second);

  for (auto itr = std::next(m_entries.begin()); itr != m_entries.end(); ++itr)
    earliest = std::min(earliest, next_time(itr->second));

  torrent::this_thread::scheduler()->update_wait_until(&m_task_process, earliest);
}

WatchReadyQueue::time_type
WatchReadyQueue::next_time(const Entry& entry) const {
  if (entry.is_nonempty_regular())
    return entry.m_last_changed + quiet_time;

  return std::min(torrent::this_thread::cached_time() + retry_time,
                  entry.m_first_seen + stale_time);
}

}
