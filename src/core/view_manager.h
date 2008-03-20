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

#ifndef RTORRENT_CORE_VIEW_MANAGER_H
#define RTORRENT_CORE_VIEW_MANAGER_H

#include <string>
#include <rak/unordered_vector.h>

#include "view.h"

namespace core {

class ViewManager : public rak::unordered_vector<View*> {
public:
  typedef rak::unordered_vector<View*> base_type;
  typedef std::list<std::string>       filter_args;
  
  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;
  
  using base_type::size_type;

  using base_type::begin;
  using base_type::end;
  using base_type::rbegin;
  using base_type::rend;

  using base_type::empty;
  using base_type::size;

  ViewManager(DownloadList* dl);
  ~ViewManager() { clear(); }

  // Ffff... Just throwing together an interface, need to think some
  // more on this.

  void                clear();

  iterator            insert(const std::string& name);
  void                insert_throw(const std::string& name) { insert(name); }

  // When erasing, just 'disable' the view so that the users won't
  // suddenly find their pointer dangling?

  iterator            find(const std::string& name);
  iterator            find_throw(const std::string& name);

  // If View::last_changed() is less than 'timeout' seconds ago, don't
  // sort.
  //
  // Find a better name for 'timeout'.
  void                sort(const std::string& name, uint32_t timeout = 0);

  // These could be moved to where the command is implemented.
  void                set_sort_new(const std::string& name, const std::string& cmd)     { (*find_throw(name))->set_sort_new(cmd); }
  void                set_sort_current(const std::string& name, const std::string& cmd) { (*find_throw(name))->set_sort_current(cmd); }

  void                set_filter(const std::string& name, const std::string& cmd);
  void                set_filter_on(const std::string& name, const filter_args& args);

  void                set_event_added(const std::string& name, const std::string& cmd)   { (*find_throw(name))->set_event_added(cmd); }
  void                set_event_removed(const std::string& name, const std::string& cmd) { (*find_throw(name))->set_event_removed(cmd); }

private:
  DownloadList*       m_list;
};

}

#endif
