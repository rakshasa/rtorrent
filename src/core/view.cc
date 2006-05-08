// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
#include <sigc++/bind.h>
#include <torrent/download.h>

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
  view_downloads_compare(const View::sort_list& s) : m_sort(s) {}

  bool operator () (Download* d1, Download* d2) const {
    for (View::sort_list::const_iterator itr = m_sort.begin(), last = m_sort.end(); itr != last; ++itr)
      if ((**itr)(d1, d2))
	return true;
      else if ((**itr)(d2, d1))
	return false;

    // Since we're testing equivalence, return false if we're
    // equal. This is a requirement for the stl sorting algorithms.
    return false;
  }

  const View::sort_list& m_sort;
};

struct view_downloads_filter : std::unary_function<Download*, bool> {
  view_downloads_filter(const View::filter_list& s) : m_filter(s) {}

  bool operator () (Download* d1) const {
    for (View::filter_list::const_iterator itr = m_filter.begin(), last = m_filter.end(); itr != last; ++itr)
      if (!(**itr)(d1))
	return false;

    // The default filter action is to return true, to not filter the
    // download out.
    return true;
  }

  const View::filter_list& m_filter;
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
  iterator split = std::stable_partition(base_type::begin(), base_type::end(), view_downloads_filter(m_filter));

  m_size = position(split);

  // Fix focus
  m_focus = std::min(m_focus, m_size);
}

void
View::set_filter_on(int event) {
  if (event == DownloadList::SLOTS_INSERT || event == DownloadList::SLOTS_ERASE || event >= DownloadList::SLOTS_MAX_SIZE)
    throw torrent::internal_error("View::filter_on(...) invalid event.");

  m_list->slots(event)["0_view_" + m_name]  = sigc::bind(sigc::mem_fun(this, &View::received), event);
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

    if (view_downloads_filter(m_filter)(download))
      insert_visible(download);
    else
      base_type::insert(end_filtered(), download);

    if (m_focus > m_size)
      throw torrent::internal_error("View::received(...) m_focus > m_size.");

    break;

  case DownloadList::SLOTS_ERASE:
    erase(itr);
    break;

  default:
    if (itr == end_filtered())
      throw torrent::internal_error("View::received(..., SLOTS_*) could not find download.");

    if (view_downloads_filter(m_filter)(download)) {
      
      if (itr < end_visible())
	return;

      // Use base_type::erase as we don't need to modify m_size nor
      // m_focus.
      base_type::erase(itr);
      insert_visible(download);

    } else {

      if (itr >= begin_filtered())
	return;

      erase(itr);
      base_type::push_back(download);
    }

    break;
  }

  m_signalChanged.emit();
}

}
