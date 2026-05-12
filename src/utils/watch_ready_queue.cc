#include "config.h"

#include "utils/watch_ready_queue.h"

#include <algorithm>
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

  if (result.second) {
    update_next_time(&entry);
    push_entry(&entry);
  } else {
    update_entry(&entry);
  }

  schedule();
}

void
WatchReadyQueue::shutdown() {
  m_active = false;
  m_entries.clear();
  m_entry_queue.clear();
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

bool
WatchReadyQueue::compare_next_time(const Entry* lhs, const Entry* rhs) {
  return lhs->m_next_time > rhs->m_next_time;
}

void
WatchReadyQueue::push_entry(Entry* entry) {
  m_entry_queue.push_back(entry);
  std::push_heap(m_entry_queue.begin(), m_entry_queue.end(), compare_next_time);
}

void
WatchReadyQueue::update_entry(Entry* entry) {
  update_next_time(entry);
  std::make_heap(m_entry_queue.begin(), m_entry_queue.end(), compare_next_time);
}

void
WatchReadyQueue::update_next_time(Entry* entry) {
  if (entry->is_nonempty_regular()) {
    entry->m_next_time = entry->m_last_changed + quiet_time;
    return;
  }

  entry->m_next_time = std::min(torrent::this_thread::cached_time() + retry_time,
                                entry->m_first_seen + stale_time);
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

  while (!m_entry_queue.empty() && m_entry_queue.front()->m_next_time <= torrent::this_thread::cached_time()) {
    std::pop_heap(m_entry_queue.begin(), m_entry_queue.end(), compare_next_time);
    Entry* entry = m_entry_queue.back();
    m_entry_queue.pop_back();

    update_status(entry);

    bool unchanged_entry = torrent::this_thread::cached_time() - entry->m_last_changed >= quiet_time;
    bool stale_entry = torrent::this_thread::cached_time() - entry->m_first_seen >= stale_time;

    if (entry->is_nonempty_regular() && unchanged_entry) {
      ready.emplace_back(entry->m_command, entry->m_path);
      std::string path = entry->m_path;
      m_entries.erase(path);
      continue;
    }

    if (!entry->is_nonempty_regular() && stale_entry) {
      std::string path = entry->m_path;
      lt_log_print(torrent::LOG_NOTICE, "Dropped unready watch file after timeout: \"%s\"", path.c_str());
      m_entries.erase(path);
      continue;
    }

    update_next_time(entry);
    push_entry(entry);
  }

  schedule();

  for (const auto& item : ready)
    rpc::commands.call_catch(item.first.c_str(), rpc::make_target(), item.second);
}

void
WatchReadyQueue::schedule() {
  if (m_entry_queue.empty()) {
    torrent::this_thread::scheduler()->erase(&m_task_process);
    return;
  }

  torrent::this_thread::scheduler()->update_wait_until(&m_task_process, m_entry_queue.front()->m_next_time);
}

}
