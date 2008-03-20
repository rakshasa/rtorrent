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
#include <rak/functional.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>

#include "globals.h"
#include "control.h"
#include "rpc/parse_commands.h"

#include "download.h"
#include "download_list.h"
#include "manager.h"
#include "view.h"
#include "view_manager.h"

namespace core {

// Really need to implement a factory and allow options in the sort
// statements.
ViewManager::ViewManager(DownloadList* dl) :
  m_list(dl) {
}

void
ViewManager::clear() {
  std::for_each(begin(), end(), rak::call_delete<View>());

  base_type::clear();
}

ViewManager::iterator
ViewManager::insert(const std::string& name) {
  if (find(name) != end())
    throw torrent::internal_error("ViewManager::insert(...) name already inserted.");

  View* view = new View();
  view->initialize(name, m_list);

  return base_type::insert(end(), view);
}

ViewManager::iterator
ViewManager::find(const std::string& name) {
  return std::find_if(begin(), end(), rak::equal(name, std::mem_fun(&View::name)));
}

ViewManager::iterator
ViewManager::find_throw(const std::string& name) {
  iterator itr = std::find_if(begin(), end(), rak::equal(name, std::mem_fun(&View::name)));

  if (itr == end())
    throw torrent::input_error("Could not find view: " + name);

  return itr;
}

void
ViewManager::sort(const std::string& name, uint32_t timeout) {
  iterator viewItr = find_throw(name);

  if ((*viewItr)->last_changed() + rak::timer::from_seconds(timeout) > cachedTime)
    return;

  // Should we rename sort, or add a seperate function?
  (*viewItr)->filter();
  (*viewItr)->sort();
}

void
ViewManager::set_filter(const std::string& name, const std::string& cmd) {
  iterator viewItr = find_throw(name);

  (*viewItr)->set_filter(cmd);
  (*viewItr)->filter();
}

void
ViewManager::set_filter_on(const std::string& name, const filter_args& args) {
  iterator viewItr = find_throw(name);

  (*viewItr)->clear_filter_on();

  for (filter_args::const_iterator itr = args.begin(); itr != args.end(); ++itr) {

    if (*itr == "start")
      (*viewItr)->set_filter_on(DownloadList::SLOTS_START);

    else if (*itr == "stop")
      (*viewItr)->set_filter_on(DownloadList::SLOTS_STOP);

    else if (*itr == "hash_queued")
      (*viewItr)->set_filter_on(DownloadList::SLOTS_HASH_QUEUED);

    else if (*itr == "hash_removed")
      (*viewItr)->set_filter_on(DownloadList::SLOTS_HASH_REMOVED);

    else if (*itr == "hash_done")
      (*viewItr)->set_filter_on(DownloadList::SLOTS_HASH_DONE);

    else if (*itr == "finished")
      (*viewItr)->set_filter_on(DownloadList::SLOTS_FINISHED);

    else
      throw torrent::input_error("Invalid filter on identifier.");

  }
}

}
