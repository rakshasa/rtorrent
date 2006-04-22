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
#include <rak/functional.h>
#include <torrent/exceptions.h>

#include "download.h"
#include "view_downloads.h"
#include "view_manager.h"

namespace core {

ViewManager::ViewManager(DownloadList* dl) :
  m_list(dl) {
}

void
ViewManager::clear() {
  std::for_each(begin(), end(), rak::call_delete<ViewDownloads>());

  base_type::clear();
}

ViewManager::iterator
ViewManager::insert(const std::string& name) {
  if (find(name) != end())
    throw torrent::internal_error("ViewManager::insert(...) name already inserted.");

  ViewDownloads* view = new ViewDownloads();
  view->initialize(name, m_list);

  return base_type::insert(end(), view);
}

ViewManager::iterator
ViewManager::find(const std::string& name) {
  return std::find_if(begin(), end(), rak::equal(name, std::mem_fun(&ViewDownloads::name)));
}

ViewManager::iterator
ViewManager::find_throw(const std::string& name) {
  iterator itr = std::find_if(begin(), end(), rak::equal(name, std::mem_fun(&ViewDownloads::name)));

  if (itr == end())
    throw torrent::input_error("Could not find view: " + name);

  return itr;
}

// Put this somewhere else.
bool
view_sort_name(core::Download* d1, core::Download* d2) {
  return d1->download()->name() < d2->download()->name();
}

bool
view_sort_name_reverse(core::Download* d1, core::Download* d2) {
  return d1->download()->name() > d2->download()->name();
}

void
ViewManager::sort(const std::string& name, const std::string& sort) {
  iterator itr = find_throw(name);

  // Quick and dirty hack.
  if (sort == "none")
    (*itr)->sort(NULL);

  else if (sort == "name")
    (*itr)->sort(&view_sort_name);

  else if (sort == "name_reverse")
    (*itr)->sort(&view_sort_name_reverse);

  else
    throw torrent::input_error("Invalid sorting identifier.");
}

void
ViewManager::set_sort_new(const std::string& name, const std::string& sort) {
  iterator itr = find_throw(name);

  // Quick and dirty hack.
  if (sort == "none")
    (*itr)->set_sort_new(NULL);

  else if (sort == "name")
    (*itr)->set_sort_new(&view_sort_name);

  else if (sort == "name_reverse")
    (*itr)->set_sort_new(&view_sort_name_reverse);

  else
    throw torrent::input_error("Invalid sorting identifier.");
}

}
