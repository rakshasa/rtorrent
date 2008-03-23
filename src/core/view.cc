// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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
#include <rpc/parse_commands.h>
#include <sigc++/adaptors/bind.h>
#include <torrent/download.h>
#include <torrent/exceptions.h>

#include "download.h"
#include "download_list.h"

#include "view.h"

namespace core {

View::~View() {
  if (m_name.empty())
    return;

  std::for_each(m_list->slot_map_begin(), m_list->slot_map_end(), rak::bind2nd(std::ptr_fun(&DownloadList::erase_key), "0_view_" + m_name));
}

void
View::initialize(const std::string& name, core::DownloadList* dlist) {
  if (!m_name.empty())
    throw torrent::internal_error("View::initialize(...) called on an already initialized view.");

  if (name.empty())
    throw torrent::internal_error("View::initialize(...) called with an empty name.");

  std::string key = "0_view_" + name;

  if (dlist->has_slot_insert(key) || dlist->has_slot_erase(key))
    throw torrent::internal_error("View::initialize(...) duplicate key name found in DownloadList.");

  m_name = name;
  m_list = dlist;

  // Urgh, wrong. No filtering being done.
  std::for_each(m_list->begin(), m_list->end(), rak::bind1st(std::mem_fun(&View::push_back), this));

  m_size = base_type::size();
  m_focus = 0;

  m_list->slot_map_insert()[key] = sigc::bind(sigc::mem_fun(this, &View::received), (int)DownloadList::SLOTS_INSERT);
  m_list->slot_map_erase()[key]  = sigc::bind(sigc::mem_fun(this, &View::received), (int)DownloadList::SLOTS_ERASE);

  set_last_changed(rak::timer());
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

  rpc::parse_command_multiple_d_nothrow(download, m_eventAdded);
}

void
View::set_not_visible(Download* download) {
  iterator itr = std::find(begin_visible(), end_visible(), download);

  if (itr == end_visible())
    return;

  // Don't optimize erase since we want to keep the order of the
  // non-visible elements.
  base_type::erase(itr);
  base_type::push_back(download);

  rpc::parse_command_multiple_d_nothrow(download, m_eventRemoved);
}

void
View::next_focus() {
  if (empty())
    return;

  m_focus = (m_focus + 1) % (size() + 1);
  m_signalChanged.emit();
}

void
View::prev_focus() {
  if (empty())
    return;

  m_focus = (m_focus - 1 + size() + 1) % (size() + 1);
  m_signalChanged.emit();
}

// Need to use wrapper-functors so it will properly call the virtual
// functions.

// Also add focus thingie here?
struct view_downloads_compare : std::binary_function<Download*, Download*, bool> {
  view_downloads_compare(const std::string& cmd) : m_command(cmd) {}

  bool operator () (Download* d1, Download* d2) const {
    try {
      if (m_command.empty())
        return false;

      return rpc::parse_command_single(rpc::make_target_pair(d1, d2), m_command).as_value();

    } catch (torrent::input_error& e) {
//       control->core()->push_log(e.what());

      return false;
    }
  }

  const std::string& m_command;
};

struct view_downloads_filter : std::unary_function<Download*, bool> {
  view_downloads_filter(const std::string& cmd) : m_command(cmd) {}

  bool operator () (Download* d1) const {
    if (m_command.empty())
      return true;

    try {
      torrent::Object result = rpc::parse_command_single(rpc::make_target(d1), m_command);

      switch (result.type()) {
      case torrent::Object::TYPE_NONE:   return false;
      case torrent::Object::TYPE_VALUE:  return result.as_value();
      case torrent::Object::TYPE_STRING: return !result.as_string().empty();
      case torrent::Object::TYPE_LIST:   return !result.as_list().empty();
      case torrent::Object::TYPE_MAP:    return !result.as_map().empty();
      }

      // The default filter action is to return true, to not filter
      // the download out.
      return true;

    } catch (torrent::input_error& e) {
      return false;
    }
  }

  const std::string&       m_command;
};

void
View::sort() {
  Download* curFocus = focus() != end_visible() ? *focus() : NULL;

  // Don't go randomly switching around equivalent elements.
  std::stable_sort(begin(), end_visible(), view_downloads_compare(m_sortCurrent));

  m_focus = position(std::find(begin(), end_visible(), curFocus));
  m_signalChanged.emit();
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
  if (!m_eventRemoved.empty())
    std::for_each(changed.begin(), splitChanged, rak::bind2nd(std::ptr_fun(&rpc::parse_command_multiple_d_nothrow), m_eventRemoved));

  if (!m_eventAdded.empty())
    std::for_each(splitChanged, changed.end(),   rak::bind2nd(std::ptr_fun(&rpc::parse_command_multiple_d_nothrow), m_eventAdded));
}

void
View::set_filter_on(int event) {
  if (event == DownloadList::SLOTS_INSERT || event == DownloadList::SLOTS_ERASE || event >= DownloadList::SLOTS_MAX_SIZE)
    throw torrent::internal_error("View::filter_on(...) invalid event.");

  m_list->slots(event)["0_view_" + m_name]  = sigc::bind(sigc::mem_fun(this, &View::received), event);
}

void
View::clear_filter_on() {
  // Don't clear insert and erase as these are required to keep the
  // View up-to-date with the available downloads.
  std::for_each(m_list->slot_map_begin() + DownloadList::SLOTS_OPEN, m_list->slot_map_end(), rak::bind2nd(std::ptr_fun(&DownloadList::erase_key), "0_view_" + m_name));
}

inline void
View::insert_visible(Download* d) {
  iterator itr = std::find_if(begin_visible(), end_visible(), std::bind1st(view_downloads_compare(m_sortNew), d));

  m_size++;
  m_focus += (m_focus >= position(itr));

  base_type::insert(itr, d);
}

inline void
View::erase(iterator itr) {
  if (itr == end_filtered())
    throw torrent::internal_error("View::erase_visible(...) iterator out of range.");

  m_size -= (itr < end_visible());
  m_focus -= (m_focus > position(itr));

  base_type::erase(itr);
}

void
View::received(core::Download* download, int event) {
  iterator itr = std::find(base_type::begin(), base_type::end(), download);

  switch (event) {
  case DownloadList::SLOTS_INSERT:
  
    if (itr != base_type::end())
      throw torrent::internal_error("View::received(..., SLOTS_INSERT) already inserted.");

    if (view_downloads_filter(m_filter)(download)) {
      insert_visible(download);
      rpc::parse_command_multiple_d_nothrow(download, m_eventAdded);

    } else {
      base_type::insert(end_filtered(), download);
      return;
    }

    if (m_focus > m_size)
      throw torrent::internal_error("View::received(...) m_focus > m_size.");

    break;

  case DownloadList::SLOTS_ERASE:
    if (itr >= end_visible()) {
      erase(itr);
      return;
    }

    erase(itr);
    rpc::parse_command_multiple_d_nothrow(download, m_eventRemoved);

    break;

  default:
    if (itr == end_filtered())
      throw torrent::internal_error("View::received(..., SLOTS_*) could not find download.");

    if (view_downloads_filter(m_filter)(download)) {
      
      if (itr >= end_visible()) {
        erase(itr);
        insert_visible(download);

        rpc::parse_command_multiple_d_nothrow(download, m_eventAdded);

      } else {
        // This makes sure the download is sorted even if it is
        // already visible.
        erase(itr);
        insert_visible(download);
      }

    } else {
      if (itr >= end_visible())
        return;

      erase(itr);
      base_type::push_back(download);

      rpc::parse_command_multiple_d_nothrow(download, m_eventRemoved);
    }

    break;
  }

  m_signalChanged.emit();
}

}
