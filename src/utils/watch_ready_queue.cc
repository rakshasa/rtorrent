#include "config.h"

#include "utils/watch_ready_queue.h"

#include <algorithm>
#include <torrent/utils/file_stat.h>
#include <torrent/utils/log.h>

#include "globals.h"
#include "rpc/rpc_manager.h"

namespace utils {

namespace {

auto compare_next_time = [](const auto* lhs, const auto* rhs) {
  return lhs->next_time > rhs->next_time;
};

}

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
    entry.path = path;
    entry.first_seen = torrent::this_thread::cached_time();
  }

  entry.command = command;
  entry.last_changed = torrent::this_thread::cached_time();
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
  if (entry->regular && entry->size > 0) {
    entry->next_time = entry->last_changed + quiet_time;
    return;
  }

  entry->next_time = std::min(torrent::this_thread::cached_time() + retry_time,
                              entry->first_seen + stale_time);
}

void
WatchReadyQueue::update_status(Entry* entry) {
  torrent::utils::FileStat fs;
  bool regular = false;
  int64_t size = -1;
  time_t mtime = 0;

  if (fs.update(expand_path(entry->path))) {
    regular = fs.is_regular();
    size = fs.size();
    mtime = fs.modified_time();
  }

  if (entry->regular != regular || entry->size != size || entry->mtime != mtime) {
    entry->regular = regular;
    entry->size = size;
    entry->mtime = mtime;
    entry->last_changed = torrent::this_thread::cached_time();
  }
}

void
WatchReadyQueue::process() {
  ready_list ready;

  while (!m_entry_queue.empty() && m_entry_queue.front()->next_time <= torrent::this_thread::cached_time()) {
    std::pop_heap(m_entry_queue.begin(), m_entry_queue.end(), compare_next_time);
    Entry* entry = m_entry_queue.back();
    m_entry_queue.pop_back();

    update_status(entry);

    bool unchanged_entry = torrent::this_thread::cached_time() - entry->last_changed >= quiet_time;
    bool stale_entry = torrent::this_thread::cached_time() - entry->first_seen >= stale_time;
    bool nonempty_regular = entry->regular && entry->size > 0;

    if (nonempty_regular && unchanged_entry) {
      ready.emplace_back(entry->command, entry->path);
      std::string path = entry->path;
      m_entries.erase(path);
      continue;
    }

    if (!nonempty_regular && stale_entry) {
      std::string path = entry->path;
      lt_log_print(torrent::LOG_SYSTEM, "system: Dropped unready watch file after timeout: \"%s\"", path.c_str());
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

  torrent::this_thread::scheduler()->update_wait_until(&m_task_process, m_entry_queue.front()->next_time);
}

}
