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

// Provides a filtered and sorted list of downloads that can be
// updated auto-magically.
//
// We don't worry about std::vector's insert/erase performance as the
// elements get accessed often but not modified, better with cache
// locality.
//
// View::m_size indicates the number of Download's that
// remain visible, e.g. has not been filtered out. The Download's that
// were filtered are still in the underlying vector, but cannot be
// accessed through the normal stl container functions.

#ifndef RTORRENT_CORE_VIEW_DOWNLOADS_H
#define RTORRENT_CORE_VIEW_DOWNLOADS_H

#include <memory>
#include <string>
#include <vector>
#include <rak/timer.h>
#include <sigc++/signal.h>

#include "globals.h"

namespace core {

class Download;
class DownloadList;

class View : private std::vector<Download*> {
public:
  typedef std::vector<Download*>         base_type;
  typedef sigc::signal0<void>            signal_type;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;
  
  using base_type::size_type;

  View() {}
  ~View();

  void                initialize(const std::string& name, DownloadList* dlist);

  const std::string&  name() const                            { return m_name; }

  bool                empty_visible() const                   { return m_size == 0; }

  size_type           size() const                            { return m_size; }

  // Perhaps this should be renamed?
  iterator            begin_visible()                         { return begin(); }
  const_iterator      begin_visible() const                   { return begin(); }

  iterator            end_visible()                           { return begin() + m_size; }
  const_iterator      end_visible() const                     { return begin() + m_size; }

  iterator            begin_filtered()                        { return begin() + m_size; }
  const_iterator      begin_filtered() const                  { return begin() + m_size; }

  iterator            end_filtered()                          { return base_type::end(); }
  const_iterator      end_filtered() const                    { return base_type::end(); }

  iterator            focus()                                 { return begin() + m_focus; }
  const_iterator      focus() const                           { return begin() + m_focus; }
  void                set_focus(iterator itr)                 { m_focus = position(itr); m_signalChanged.emit(); }

  void                insert(Download* download);
  void                erase(Download* download);

  void                set_visible(Download* download);
  void                set_not_visible(Download* download);

  void                next_focus();
  void                prev_focus();

  void                sort();

  void                set_sort_new(const std::string& s)      { m_sortNew = s; }
  void                set_sort_current(const std::string& s)  { m_sortCurrent = s; }

  // Need to explicity trigger filtering.
  void                filter();
  void                filter_download(core::Download* download);

  void                set_filter(const std::string& s)        { m_filter = s; }
  void                set_filter_on(int event);

  void                clear_filter_on();

  void                set_event_added(const std::string& cmd)   { m_eventAdded = cmd; }
  void                set_event_removed(const std::string& cmd) { m_eventRemoved = cmd; }

  // The time of the last change to the view, semantics of this is
  // user-dependent. Used by f.ex. ViewManager to decide if it should
  // sort and/or filter a view.
  //
  // Currently initialized to rak::timer(), though perhaps we should
  // use cachedTimer.
  rak::timer          last_changed() const                                 { return m_lastChanged; }
  void                set_last_changed(const rak::timer& t = ::cachedTime) { m_lastChanged = t; }

  // Don't connect any slots until after initialize else it get's
  // triggered when adding the Download's in DownloadList.
  signal_type&        signal_changed()                        { return m_signalChanged; }

private:
  View(const View&);
  void operator = (const View&);

  void                push_back(Download* d)                  { base_type::push_back(d); }

  inline void         insert_visible(Download* d);
  inline void         erase_internal(iterator itr);

  size_type           position(const_iterator itr) const      { return itr - begin(); }

  // An received thing for changed status so we can sort and filter.

  std::string         m_name;

  DownloadList*       m_list;

  size_type           m_size;
  size_type           m_focus;

  // These should be replaced by a faster non-string command type.
  std::string         m_sortNew;
  std::string         m_sortCurrent;

  std::string         m_filter;

  std::string         m_eventAdded;
  std::string         m_eventRemoved;

  rak::timer          m_lastChanged;
  signal_type         m_signalChanged;
};

}

#endif
