// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <algorithm>
#include <functional>
#include <rak/functional.h>
#include <rak/functional_fun.h>
#include <torrent/download.h>
#include <torrent/exceptions.h>

#include "rpc/parse_commands.h"
#include "rpc/object_storage.h"
#include "control.h"
#include "download.h"
#include "download_list.h"
#include "manager.h"
#include "view.h"

namespace core {

// Also add focus thingie here?
struct view_downloads_compare : std::binary_function<Download*, Download*, bool> {
  view_downloads_compare(const torrent::Object& cmd) : m_command(cmd) {}

  bool operator () (Download* d1, Download* d2) const {
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

      return rpc::commands.call_command(m_command.as_dict_key().c_str(), m_command.as_dict_obj(),
                                        rpc::make_target_pair(d1, d2)).as_value();

    } catch (torrent::input_error& e) {
      control->core()->push_log(e.what());

      return false;
    }
  }

  const torrent::Object& m_command;
};

struct view_downloads_filter : std::unary_function<Download*, bool> {
  view_downloads_filter(const torrent::Object& cmd) : m_command(cmd) {}

  bool operator () (Download* d1) const {
    if (m_command.is_empty())
      return true;

    try {
      torrent::Object result;

      if (m_command.is_dict_key()) {
        // torrent::Object tmp_command = m_command;

        // uint32_t flags = tmp_command.flags() & torrent::Object::mask_function;
        // tmp_command.unset_flags(torrent::Object::mask_function);
        // tmp_command.set_flags((flags >> 1) & torrent::Object::mask_function);

        // rpc::parse_command_execute(rpc::make_target(d1), &tmp_command);
        // result = rpc::commands.call_command(tmp_command.as_dict_key().c_str(), tmp_command.as_dict_obj(),
        //                                     rpc::make_target(d1));

        result = rpc::commands.call_command(m_command.as_dict_key().c_str(), m_command.as_dict_obj(), rpc::make_target(d1));

      } else {
        result = rpc::parse_command_single(rpc::make_target(d1), m_command.as_string());
      }

      switch (result.type()) {
        //      case torrent::Object::TYPE_RAW_BENCODE: return !result.as_raw_bencode().empty();
      case torrent::Object::TYPE_VALUE:       return result.as_value();
      case torrent::Object::TYPE_STRING:      return !result.as_string().empty();
      case torrent::Object::TYPE_LIST:        return !result.as_list().empty();
      case torrent::Object::TYPE_MAP:         return !result.as_map().empty();
      default: return false;
      }

      // The default filter action is to return true, to not filter
      // the download out.
      return true;

    } catch (torrent::input_error& e) {
      control->core()->push_log(e.what());

      return false;
    }
  }

  const torrent::Object&       m_command;
};

void
View::emit_changed() {
  priority_queue_erase(&taskScheduler, &m_delayChanged);
  priority_queue_insert(&taskScheduler, &m_delayChanged, cachedTime);
}

void
View::emit_changed_now() {
  for (signal_void::iterator itr = m_signal_changed.begin(), last = m_signal_changed.end(); itr != last; itr++)
    (*itr)();
}

View::~View() {
  if (m_name.empty())
    return;

  clear_filter_on();
  priority_queue_erase(&taskScheduler, &m_delayChanged);
}

void
View::initialize(const std::string& name) {
  if (!m_name.empty())
    throw torrent::internal_error("View::initialize(...) called on an already initialized view.");

  if (name.empty())
    throw torrent::internal_error("View::initialize(...) called with an empty name.");

  core::DownloadList* dlist = control->core()->download_list();

  m_name = name;

  // Urgh, wrong. No filtering being done.
  std::for_each(dlist->begin(), dlist->end(), rak::bind1st(std::mem_fun(&View::push_back), this));

  m_size = base_type::size();
  m_focus = 0;

  set_last_changed(rak::timer());
  m_delayChanged.slot() = std::bind(&View::emit_changed_now, this);
}

void
View::erase(Download* download) {
  iterator itr = std::find(base_type::begin(), base_type::end(), download);

  if (itr >= end_visible()) {
    erase_internal(itr);

  } else {
    erase_internal(itr);
    rpc::call_object_nothrow(m_event_removed, rpc::make_target(download));
  }
}

void
View::set_visible(Download* download) {
  iterator itr = std::find(begin_filtered(), end_filtered(), download);

  if (itr == end_filtered())
    return;

  // Don't optimize erase since we want to keep the order of the
  // non-visible elements.
  base_type::erase(itr);
  insert_visible(download);

  rpc::call_object_nothrow(m_event_added, rpc::make_target(download));
}

void
View::set_not_visible(Download* download) {
  iterator itr = std::find(begin_visible(), end_visible(), download);

  if (itr == end_visible())
    return;

  m_size--;
  m_focus -= (m_focus > position(itr));

  // Don't optimize erase since we want to keep the order of the
  // non-visible elements.
  base_type::erase(itr);
  base_type::push_back(download);

  rpc::call_object_nothrow(m_event_removed, rpc::make_target(download));
}

void
View::next_focus() {
  if (empty())
    return;

  m_focus = (m_focus + 1) % (size() + 1);
  emit_changed();
}

void
View::prev_focus() {
  if (empty())
    return;

  m_focus = (m_focus - 1 + size() + 1) % (size() + 1);
  emit_changed();
}

void
View::sort() {
  Download* curFocus = focus() != end_visible() ? *focus() : NULL;

  // Don't go randomly switching around equivalent elements.
  std::stable_sort(begin(), end_visible(), view_downloads_compare(m_sortCurrent));

  m_focus = position(std::find(begin(), end_visible(), curFocus));
  emit_changed();
}

void
View::filter() {
  // Parition the list in two steps so we know which elements changed.
  iterator splitVisible  = std::stable_partition(begin_visible(),  end_visible(),  view_downloads_filter(m_filter));
  iterator splitFiltered = std::stable_partition(begin_filtered(), end_filtered(), view_downloads_filter(m_filter));

  base_type changed(splitVisible, splitFiltered);
  iterator splitChanged = changed.begin() + std::distance(splitVisible, end_visible());
  
  m_size = std::distance(begin(), std::copy(splitChanged, changed.end(), splitVisible));
  std::copy(changed.begin(), splitChanged, begin_filtered());

  // Fix this...
  m_focus = std::min(m_focus, m_size);

  // The commands are allowed to remove itself from or change View
  // sorting since the commands are being called on the 'changed'
  // vector. But this will cause undefined behavior if elements are
  // removed.
  //
  // Consider if View should lock itself (and throw) if erase events
  // are triggered on a Download in the 'changed' list. This can be
  // done by using a base_type* member variable, and making sure we
  // set the elements to NULL as we trigger commands on them. Or
  // perhaps always clear them, thus not throwing anything.
  if (!m_event_removed.is_empty())
    std::for_each(changed.begin(), splitChanged,
                  std::bind(&rpc::call_object_d_nothrow, m_event_removed, std::placeholders::_1));

  if (!m_event_added.is_empty())
    std::for_each(changed.begin(), splitChanged,
                  std::bind(&rpc::call_object_d_nothrow, m_event_added, std::placeholders::_1));

  emit_changed();
}

void
View::filter_download(core::Download* download) {
  iterator itr = std::find(base_type::begin(), base_type::end(), download);

  if (itr == base_type::end())
    throw torrent::internal_error("View::filter_download(...) could not find download.");

  if (view_downloads_filter(m_filter)(download)) {
      
    if (itr >= end_visible()) {
      erase_internal(itr);
      insert_visible(download);

      rpc::call_object_nothrow(m_event_added, rpc::make_target(download));

    } else {
      // This makes sure the download is sorted even if it is
      // already visible.
      //
      // Consider removing this.
      erase_internal(itr);
      insert_visible(download);
    }

  } else {
    if (itr >= end_visible())
      return;

    erase_internal(itr);
    base_type::push_back(download);

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

inline void
View::insert_visible(Download* d) {
  iterator itr = std::find_if(begin_visible(), end_visible(), std::bind1st(view_downloads_compare(m_sortNew), d));

  m_size++;
  m_focus += (m_focus >= position(itr));

  base_type::insert(itr, d);
}

inline void
View::erase_internal(iterator itr) {
  if (itr == end_filtered())
    throw torrent::internal_error("View::erase_visible(...) iterator out of range.");

  m_size -= (itr < end_visible());
  m_focus -= (m_focus > position(itr));

  base_type::erase(itr);
}

}
