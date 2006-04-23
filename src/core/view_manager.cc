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

class ViewSortFirst : public ViewSort {
public:
  virtual bool compare(Download* d1, Download* d2) const {
    return true;
  }
};

class ViewSortName : public ViewSort {
public:
  virtual bool compare(Download* d1, Download* d2) const {
    return d1->download()->name() < d2->download()->name();
  }
};

class ViewSortVariable : public ViewSort {
public:
  ViewSortVariable(const std::string& name, const std::string& value) :
    m_name(name), m_value(value) {}

  virtual bool compare(Download* d1, Download* d2) const {
    return
      d1->variable()->get_string(m_name) == m_value ||
      d2->variable()->get_string(m_name) != m_value;
  }

private:
  std::string m_name;
  std::string m_value;
};

class ViewSortReverse : public ViewSort {
public:
  ViewSortReverse(ViewSort* s) : m_sort(s) {}
  ~ViewSortReverse() { delete m_sort; }

  virtual bool compare(Download* d1, Download* d2) const {
    return m_sort->compare(d2, d1);
  }

private:
  ViewSort* m_sort;
};

// Hmm, use a list in ViewDownloads instead?
class ViewSortAnd : public ViewSort {
public:
  ViewSortAnd(ViewSort* s1, ViewSort* s2) : m_sort1(s1), m_sort2(s2) {}
  ~ViewSortAnd() { delete m_sort1; delete m_sort2; }

  virtual bool compare(Download* d1, Download* d2) const {
    return m_sort1->compare(d1, d2) && m_sort2->compare(d1, d2);
  }

private:
  ViewSort* m_sort1;
  ViewSort* m_sort2;
};

ViewManager::ViewManager(DownloadList* dl) :
  m_list(dl) {

  m_sort["first"]        = new ViewSortFirst();
  m_sort["last"]         = new ViewSortReverse(new ViewSortFirst());
  m_sort["name"]         = new ViewSortName();
  m_sort["name_reverse"] = new ViewSortReverse(new ViewSortName());

  m_sort["started"]      = new ViewSortVariable("state", "started");
  m_sort["started_name"] = new ViewSortAnd(new ViewSortVariable("state", "started"), new ViewSortName());

  m_sort["stopped"]      = new ViewSortVariable("state", "stopped");
  m_sort["stopped_name"] = new ViewSortAnd(new ViewSortVariable("state", "stopped"), new ViewSortName());
}

void
ViewManager::clear() {
  std::for_each(begin(), end(), rak::call_delete<ViewDownloads>());
  std::for_each(m_sort.begin(), m_sort.end(), rak::on(rak::mem_ptr_ref(&sort_map::value_type::second), rak::call_delete<ViewSort>()));

  base_type::clear();
}

ViewManager::iterator
ViewManager::insert(const std::string& name) {
  if (find(name) != end())
    throw torrent::internal_error("ViewManager::insert(...) name already inserted.");

  ViewDownloads* view = new ViewDownloads();

  view->set_sort_new(m_sort["last"]);
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

void
ViewManager::sort(const std::string& name, const std::string& sort) {
  iterator viewItr = find_throw(name);

  sort_map::const_iterator sortItr = m_sort.find(sort);

  if (sortItr == m_sort.end())
    throw torrent::input_error("Invalid sorting identifier.");

  (*viewItr)->sort(sortItr->second);
}

void
ViewManager::set_sort_new(const std::string& name, const std::string& sort) {
  iterator viewItr = find_throw(name);

  sort_map::const_iterator sortItr = m_sort.find(sort);

  if (sortItr == m_sort.end())
    throw torrent::input_error("Invalid sorting identifier.");

  (*viewItr)->set_sort_new(sortItr->second);
}

}
