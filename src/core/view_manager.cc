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

#include "globals.h"

#include "download.h"
#include "view.h"
#include "view_manager.h"

namespace core {

class ViewSortName : public ViewSort {
public:
  virtual bool operator () (Download* d1, Download* d2) const {
    return d1->download()->name() < d2->download()->name();
  }
};

class ViewSortVariable : public ViewSort {
public:
  ViewSortVariable(const std::string& name, const std::string& value) :
    m_name(name), m_value(value) {}

  virtual bool operator () (Download* d1, Download* d2) const {
    return
      d1->variable()->get_string(m_name) == m_value &&
      d2->variable()->get_string(m_name) != m_value;
  }

private:
  std::string m_name;
  std::string m_value;
};

class ViewSortVariableValue : public ViewSort {
public:
  ViewSortVariableValue(const std::string& name) :
    m_name(name) {}

  virtual bool operator () (Download* d1, Download* d2) const {
    return d1->variable()->get_value(m_name) < d2->variable()->get_value(m_name);
  }

private:
  std::string m_name;
};

class ViewSortReverse : public ViewSort {
public:
  ViewSortReverse(ViewSort* s) : m_sort(s) {}
  ~ViewSortReverse() { delete m_sort; }

  virtual bool operator () (Download* d1, Download* d2) const {
    return (*m_sort)(d2, d1);
  }

private:
  ViewSort* m_sort;
};

class ViewFilterVariableValue : public ViewFilter {
public:
  ViewFilterVariableValue(const std::string& name, torrent::Object::value_type v) :
    m_name(name), m_value(v) {}

  virtual bool operator () (Download* d1) const {
    return d1->variable()->get_value(m_name) == m_value;
  }

private:
  std::string                 m_name;
  torrent::Object::value_type m_value;
};

// Really need to implement a factory and allow options in the sort
// statements.
ViewManager::ViewManager(DownloadList* dl) :
  m_list(dl) {

//   m_sort["first"]        = new ViewSortNot(new ViewSort());
//   m_sort["last"]         = new ViewSort();
  m_sort["name"]          = new ViewSortName();
  m_sort["name_reverse"]  = new ViewSortReverse(new ViewSortName());

  m_sort["stopped"]       = new ViewSortVariableValue("state");
  m_sort["started"]       = new ViewSortReverse(new ViewSortVariableValue("state"));
  m_sort["complete"]      = new ViewSortVariableValue("complete");
  m_sort["incomplete"]    = new ViewSortReverse(new ViewSortVariableValue("complete"));

  m_sort["state_changed"]         = new ViewSortVariableValue("state_changed");
  m_sort["state_changed_reverse"] = new ViewSortReverse(new ViewSortVariableValue("state_changed"));

  m_filter["started"]     = new ViewFilterVariableValue("state", 1);
  m_filter["stopped"]     = new ViewFilterVariableValue("state", 0);
  m_filter["complete"]    = new ViewFilterVariableValue("complete", 1);
  m_filter["incomplete"]  = new ViewFilterVariableValue("complete", 0);
}

void
ViewManager::clear() {
  std::for_each(begin(), end(), rak::call_delete<View>());
  std::for_each(m_sort.begin(), m_sort.end(), rak::on(rak::mem_ptr_ref(&sort_map::value_type::second), rak::call_delete<ViewSort>()));

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

inline ViewManager::sort_list
ViewManager::build_sort_list(const sort_args& args) {
  View::sort_list sortList;
  sortList.reserve(args.size());

  for (sort_args::const_iterator itr = args.begin(), last = args.end(); itr != last; ++itr) {
    sort_map::const_iterator sortItr = m_sort.find(*itr);

    if (sortItr == m_sort.end())
      throw torrent::input_error("Invalid sorting identifier.");

    sortList.push_back(sortItr->second);
  }

  return sortList;
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
ViewManager::set_sort_new(const std::string& name, const sort_args& sort) {
  iterator viewItr = find_throw(name);

  (*viewItr)->set_sort_new(build_sort_list(sort));
}

void
ViewManager::set_sort_current(const std::string& name, const sort_args& sort) {
  iterator viewItr = find_throw(name);

  (*viewItr)->set_sort_current(build_sort_list(sort));
}

inline ViewManager::filter_list
ViewManager::build_filter_list(const filter_args& args) {
  View::filter_list filterList;
  filterList.reserve(args.size());

  for (filter_args::const_iterator itr = args.begin(), last = args.end(); itr != last; ++itr) {
    filter_map::const_iterator filterItr = m_filter.find(*itr);

    if (filterItr == m_filter.end())
      throw torrent::input_error("Invalid filtering identifier.");

    filterList.push_back(filterItr->second);
  }

  return filterList;
}

void
ViewManager::set_filter(const std::string& name, const filter_args& args) {
  iterator viewItr = find_throw(name);

  (*viewItr)->set_filter(build_filter_list(args));
}

}
