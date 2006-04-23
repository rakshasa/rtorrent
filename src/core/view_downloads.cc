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
#include <torrent/download.h>

#include "download.h"
#include "download_list.h"

#include "view_downloads.h"

namespace core {

ViewDownloads::~ViewDownloads() {
  if (m_name.empty())
    return;

  std::string key = "0_view_" + m_name;

  m_list->slot_map_insert().erase(key);
  m_list->slot_map_erase().erase(key);
}

void
ViewDownloads::initialize(const std::string& name, core::DownloadList* list) {
  if (!m_name.empty())
    throw torrent::internal_error("ViewDownloads::initialize(...) called on an already initialized view.");

  if (name.empty())
    throw torrent::internal_error("ViewDownloads::initialize(...) called with an empty name.");

  std::string key = "0_view_" + name;

  if (list->has_slot_insert(key) || list->has_slot_erase(key))
    throw torrent::internal_error("ViewDownloads::initialize(...) duplicate key name found in DownloadList.");

  m_name = name;

  m_list = list;
  m_focus = 0;

  std::for_each(m_list->begin(), m_list->end(), std::bind1st(std::mem_fun(&ViewDownloads::received_insert), this));

  m_list->slot_map_insert()[key] = sigc::mem_fun(this, &ViewDownloads::received_insert);
  m_list->slot_map_erase()[key]  = sigc::mem_fun(this, &ViewDownloads::received_erase);
}

void
ViewDownloads::next_focus() {
  if (empty())
    return;

  m_focus = (m_focus + 1) % (size() + 1);

  m_signalChanged.emit();
}

void
ViewDownloads::prev_focus() {
  if (empty())
    return;

  m_focus = (m_focus - 1 + size() + 1) % (size() + 1);

  m_signalChanged.emit();
}

// Also add focus thingie here.
struct view_downloads_compare : std::binary_function<Download*, Download*, bool> {
  view_downloads_compare(const ViewSort* s) : m_sort(s) {}

  bool operator () (Download* d1, Download* d2) const {
    return m_sort->compare(d1, d2);
  }

  const ViewSort* m_sort;
};

void
ViewDownloads::sort(const ViewSort* s) {
  Download* curFocus = focus() != end() ? *focus() : NULL;

  std::sort(begin(), end(), view_downloads_compare(s));

  m_focus = position(std::find(begin(), end(), curFocus));
  m_signalChanged.emit();
}

void
ViewDownloads::received_insert(core::Download* d) {
  iterator itr = std::find_if(begin(), end(), std::bind1st(view_downloads_compare(m_sortNew), d));

  if (m_focus >= position(itr))
    m_focus++;

  base_type::insert(itr, d);
  m_signalChanged.emit();
}

void
ViewDownloads::received_erase(core::Download* d) {
  iterator itr = std::find(begin(), end(), d);

  if (itr == end())
    return;

  if (m_focus > position(itr))
    m_focus--;

  base_type::erase(itr);
  m_signalChanged.emit();
}

}
