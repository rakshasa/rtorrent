#include "config.h"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <torrent/download.h>
#include <torrent/exceptions.h>

#include "control.h"
#include "download.h"
#include "download_list.h"
#include "manager.h"
#include "rpc/object_storage.h"
#include "rpc/parse_commands.h"
#include "view.h"

namespace core {

// Also add focus thingie here?
// Matches the list entry that owns a given download.
inline auto
entry_is(Download* download) {
  return [download](const std::shared_ptr<Download>& entry) { return entry.get() == download; };
}

struct view_downloads_compare {
  view_downloads_compare(const torrent::Object& cmd) :
      m_command(cmd) {}

  bool operator()(const std::shared_ptr<Download>& d1, const std::shared_ptr<Download>& d2) const {
    return (*this)(d1.get(), d2.get());
  }

  bool operator()(Download* d1, Download* d2) const {
    try {
      if (m_command.is_empty())
        return false;

      if (!m_command.is_dict_key())
        return rpc::parse_command_single(rpc::make_target_pair(d1, d2), m_command.as_string()).as_value();

      // torrent::Object tmp_command = m_command;

      // uint32_t flags = tmp_command.flags() & torrent::Object::mask_function;
      // tmp_command.unset_flags(torrent::Object::mask_function);
      // tmp_command.set_flags((flags >> 1) & torrent::Object::mask_function);

      // rpc::parse_command_execute(rpc::make_target_pair(d1, d2), &tmp_command);
      // return rpc::commands.call_command(tmp_command.as_dict_key().c_str(), tmp_command.as_dict_obj(),
      //                                   rpc::make_target_pair(d1, d2)).as_value();

      return rpc::commands.call_command(m_command.as_dict_key().c_str(), m_command.as_dict_obj(), rpc::make_target_pair(d1, d2)).as_value();

    } catch (torrent::input_error& e) {
      control->core()->push_log(e.what());

      return false;
    }
  }

  const torrent::Object m_command;
};

struct view_downloads_filter {
  view_downloads_filter(const torrent::Object& cmd, const torrent::Object& cmd2) :
      m_command(cmd), m_command2(cmd2) {}

  bool operator()(const std::shared_ptr<Download>& d1) const {
    return (*this)(d1.get());
  }

  bool operator()(Download* d1) const {
    const auto lifetime = d1->lifetime();
    return this->evalCmd(m_command, d1) && !lifetime.expired() && this->evalCmd(m_command2, d1);
  }

  bool evalCmd(const torrent::Object& cmd, Download* d1) const {
    if (cmd.is_empty())
      return true;

    try {
      torrent::Object result;

      if (cmd.is_dict_key()) {
        // torrent::Object tmp_command = cmd;

        // uint32_t flags = tmp_command.flags() & torrent::Object::mask_function;
        // tmp_command.unset_flags(torrent::Object::mask_function);
        // tmp_command.set_flags((flags >> 1) & torrent::Object::mask_function);

        // rpc::parse_command_execute(rpc::make_target(d1), &tmp_command);
        // result = rpc::commands.call_command(tmp_command.as_dict_key().c_str(), tmp_command.as_dict_obj(),
        //                                     rpc::make_target(d1));

        result = rpc::commands.call_command(cmd.as_dict_key().c_str(), cmd.as_dict_obj(), rpc::make_target(d1));

      } else {
        result = rpc::parse_command_single(rpc::make_target(d1), cmd.as_string());
      }

      switch (result.type()) {
        //      case torrent::Object::TYPE_RAW_BENCODE: return !result.as_raw_bencode().empty();
      case torrent::Object::TYPE_VALUE:
        return result.as_value();
      case torrent::Object::TYPE_STRING:
        return !result.as_string().empty();
      case torrent::Object::TYPE_LIST:
        return !result.as_list().empty();
      case torrent::Object::TYPE_MAP:
        return !result.as_map().empty();
      default:
        return false;
      }

      // The default filter action is to return true, to not filter
      // the download out.
      return true;

    } catch (torrent::input_error& e) {
      control->core()->push_log(e.what());

      return false;
    }
  }

  const torrent::Object m_command;
  const torrent::Object m_command2;
};

void
View::emit_changed() {
  torrent::this_thread::scheduler()->update_wait_for(&m_delay_changed, 0ms);
}

void
View::emit_changed_now() {
  for (auto& itr : m_signal_changed)
    itr();
}

View::~View() {
  if (m_name.empty())
    return;

  clear_filter_on();
  torrent::this_thread::scheduler()->erase(&m_delay_changed);
}

void
View::initialize(const std::string& name) {
  if (!m_name.empty())
    throw torrent::internal_error("View::initialize(...) called on an already initialized view.");

  if (name.empty())
    throw torrent::internal_error("View::initialize(...) called with an empty name.");

  m_name = name;

  // Urgh, wrong. No filtering being done.
  for (const auto& d : *control->core()->download_list())
    // Removal callbacks can create views before the list releases the entry.
    if (!d->is_erasing())
      push_back(d);

  m_size  = base_type::size();
  m_focus = 0;

  m_delay_changed.slot() = [this]() { emit_changed_now(); };
}

void
View::erase(Download* download) {
  iterator itr = std::find_if(base_type::begin(), base_type::end(), entry_is(download));

  if (itr >= end_visible()) {
    erase_internal(itr);

  } else {
    erase_internal(itr);
    rpc::call_object_nothrow(m_event_removed, rpc::make_target(download));
  }
}

void
View::set_visible(Download* download) {
  iterator itr = std::find_if(begin_filtered(), end_filtered(), entry_is(download));

  if (itr == end_filtered())
    return;

  // Don't optimize erase since we want to keep the order of the
  // non-visible elements.
  auto entry = *itr;

  if (insert_visible(entry))
    rpc::call_object_nothrow(m_event_added, rpc::make_target(download));
}

void
View::set_not_visible(Download* download) {
  iterator itr = std::find_if(begin_visible(), end_visible(), entry_is(download));

  if (itr == end_visible())
    return;

  m_size--;
  m_focus -= (m_focus > position(itr));

  // Don't optimize erase since we want to keep the order of the
  // non-visible elements.
  auto entry = *itr;

  base_type::erase(itr);
  base_type::push_back(entry);
  ++m_revision;

  rpc::call_object_nothrow(m_event_removed, rpc::make_target(download));
}

void
View::next_focus(unsigned int i) {
  if (empty_visible())
    return;

  // If at the boundary, roll over
  if (m_focus == size() - 1) {
    m_focus = size();
    emit_changed();
    return;
  }

  // Move forward, stop at the boundary
  if (m_focus == size()) // Needs special handling to ensure it's not off by one
    m_focus = i - 1;
  else
    m_focus += i;
  if (m_focus > size() - 1)
    m_focus = size() - 1;

  emit_changed();
}

void
View::prev_focus(unsigned int i) {
  if (empty_visible())
    return;

  // If at the boundary, roll over
  if (m_focus == size()) {
    m_focus = size() - 1;
    emit_changed();
    return;
  }

  // Move backward, stop at the boundary
  m_focus -= i;
  if (m_focus < 0 || m_focus > size())
    m_focus = size();

  emit_changed();
}

void
View::sort() {
  Download* curFocus = focus() != end_visible() ? focus()->get() : nullptr;
  const auto revision = m_revision;
  base_type sorted(begin_visible(), end_visible());
  const view_downloads_compare compare(m_sortCurrent);
  struct Changed {};
  try {
    // Comparators may erase or move downloads. Sort an owned snapshot and
    // abandon publication as soon as a callback changes this view.
    std::stable_sort(sorted.begin(), sorted.end(), [&](const auto& left, const auto& right) {
      if (revision != m_revision || left->lifetime().expired() || right->lifetime().expired())
        throw Changed{};
      const bool result = compare(left, right);
      if (revision != m_revision || left->lifetime().expired() || right->lifetime().expired())
        throw Changed{};
      return result;
    });
  } catch (const Changed&) {
    emit_changed();
    return;
  }
  std::copy(sorted.begin(), sorted.end(), begin_visible());
  ++m_revision;
  m_focus = position(std::find_if(begin(), end_visible(), entry_is(curFocus)));
  emit_changed();
}

void
View::filter() {
  if (m_name == "started" || m_name == "stopped") return;
  const base_type snapshot(begin(), end_filtered());
  const view_downloads_filter matches(m_filter, m_temp_filter);
  std::unordered_map<Download*, bool> selected;
  for (const auto& entry : snapshot) {
    if (entry->lifetime().expired()) continue;
    const bool result = matches(entry.get());
    if (!entry->lifetime().expired()) selected.emplace(entry.get(), result);
  }
  // Publish the stable partition before notifications, as before. Evaluate
  // callbacks on owned snapshots so they cannot invalidate vector iterators.
  base_type visible, hidden, added, removed;
  for (auto itr = begin(); itr != end_filtered(); ++itr) {
    const auto choice = selected.find(itr->get());
    const bool was_visible = itr < end_visible();
    const bool is_visible = choice == selected.end() ? was_visible : choice->second;
    (is_visible ? visible : hidden).push_back(*itr);
    if (was_visible != is_visible) (is_visible ? added : removed).push_back(*itr);
  }
  base_type::assign(visible.begin(), visible.end());
  base_type::insert(base_type::end(), hidden.begin(), hidden.end());
  m_size = visible.size();
  ++m_revision;
  m_focus = std::min(m_focus, m_size);
  const auto removed_event = m_event_removed, added_event = m_event_added;
  for (const auto& entry : removed)
    if (!entry->lifetime().expired() && std::find(begin_filtered(), end_filtered(), entry) != end_filtered())
      rpc::call_object_d_nothrow(removed_event, entry.get());
  for (const auto& entry : added)
    if (!entry->lifetime().expired() && std::find(begin_visible(), end_visible(), entry) != end_visible())
      rpc::call_object_d_nothrow(added_event, entry.get());
  emit_changed();
}

void
View::filter_by(const torrent::Object& condition, View::base_type& result) {
  const base_type snapshot(begin_visible(), end_visible());
  const view_downloads_filter matches(condition, m_temp_filter);
  for (const auto& entry : snapshot) {
    if (entry->lifetime().expired()) continue;
    if (matches(entry.get()) && !entry->lifetime().expired() &&
        std::find(begin_visible(), end_visible(), entry) != end_visible())
      result.push_back(entry);
  }
}

void
View::filter_download(core::Download* download) {
  auto itr = std::find_if(base_type::begin(), base_type::end(), entry_is(download));
  if (itr == base_type::end())
    throw torrent::internal_error("View::filter_download(...) could not find download.");

  const auto entry = *itr;
  const auto lifetime = download->lifetime();
  const bool matches = view_downloads_filter(m_filter, m_temp_filter)(download);
  if (lifetime.expired()) return;
  // User filters may change this vector, including through another download.
  itr = std::find_if(base_type::begin(), base_type::end(), entry_is(download));
  if (itr == base_type::end()) return;
  const bool was_visible = itr < end_visible();
  if (matches) {
    if (!insert_visible(entry)) return;
    if (!was_visible)
      rpc::call_object_nothrow(m_event_added, rpc::make_target(download));
  } else {
    if (!was_visible) return;
    erase_internal(itr);
    base_type::push_back(entry);
    rpc::call_object_nothrow(m_event_removed, rpc::make_target(download));
  }
  emit_changed();
}

void
View::set_filter_on_event(const std::string& event) {
  control->object_storage()->set_str_multi_key(event, "!view." + m_name, "view.filter_download=" + m_name);
}

void
View::clear_filter_on() {
  control->object_storage()->rlookup_clear("!view." + m_name);
}

inline bool
View::insert_visible(const std::shared_ptr<Download>& d) {
  const auto lifetime = d->lifetime();
  const base_type candidates(begin_visible(), end_visible());
  const view_downloads_compare compare(m_sortNew);
  std::shared_ptr<Download> anchor;
  // Keep the target in this view until callbacks finish: ordinary erase must
  // still find it. Never carry vector iterators across a user comparator.
  for (const auto& candidate : candidates) {
    if (candidate == d || candidate->lifetime().expired()) continue;
    const bool before = compare(d, candidate);
    if (lifetime.expired()) return false;
    if (before && !candidate->lifetime().expired()) {
      anchor = candidate;
      break;
    }
  }
  auto current = std::find_if(begin(), end_filtered(), entry_is(d.get()));
  if (current == end_filtered()) return false;
  erase_internal(current);
  const auto position = anchor ? std::find(begin_visible(), end_visible(), anchor) : end_visible();
  m_focus += (m_focus >= this->position(position));
  ++m_size;
  base_type::insert(position, d);
  ++m_revision;
  return true;
}

inline void
View::erase_internal(iterator itr) {
  if (itr == end_filtered())
    throw torrent::internal_error("View::erase_visible(...) iterator out of range.");

  m_size -= (itr < end_visible());
  m_focus -= (m_focus > position(itr));

  base_type::erase(itr);
  ++m_revision;
}

} // namespace core
