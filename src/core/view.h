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

#include <string>
#include <vector>
#include <rak/timer.h>
#include <torrent/object.h>
#include <tr1/functional>

#include "globals.h"

namespace core {

class Download;

class View : private std::vector<Download*> {
public:
  typedef std::vector<Download*>      base_type;
  typedef std::tr1::function<void ()> slot_void;
  typedef std::list<slot_void>        signal_void;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::reverse_iterator;
  using base_type::const_reverse_iterator;
  
  using base_type::size_type;

  View() {}
  ~View();

  void                initialize(const std::string& name);

  const std::string&  name() const                            { return m_name; }

  bool                empty_visible() const                   { return m_size == 0; }

  size_type           size() const                            { return m_size; }
  size_type           size_visible() const                    { return m_size; }
  size_type           size_not_visible() const                { return base_type::size() - m_size; }

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
  void                set_focus(iterator itr)                 { m_focus = position(itr); emit_changed(); }

  void                insert(Download* download)              { base_type::push_back(download); }
  void                erase(Download* download);

  void                set_visible(Download* download);
  void                set_not_visible(Download* download);

  void                next_focus();
  void                prev_focus();

  void                sort();

  void                set_sort_new(const torrent::Object& s)      { m_sortNew = s; }
  void                set_sort_current(const torrent::Object& s)  { m_sortCurrent = s; }

  // Need to explicity trigger filtering.
  void                filter();
  void                filter_download(core::Download* download);

  const torrent::Object& get_filter() const { return m_filter; }
  void                set_filter(const torrent::Object& s)        { m_filter = s; }
  void                set_filter_on_event(const std::string& event);

  void                clear_filter_on();

  const torrent::Object& event_added() const                           { return m_event_added; }
  const torrent::Object& event_removed() const                         { return m_event_removed; }
  void                   set_event_added(const torrent::Object& cmd)   { m_event_added = cmd; }
  void                   set_event_removed(const torrent::Object& cmd) { m_event_removed = cmd; }

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
  signal_void&        signal_changed()                        { return m_signal_changed; }

private:
  View(const View&);
  void operator = (const View&);

  void                push_back(Download* d)                  { base_type::push_back(d); }

  inline void         insert_visible(Download* d);
  inline void         erase_internal(iterator itr);

  void                emit_changed();
  void                emit_changed_now();

  size_type           position(const_iterator itr) const      { return itr - begin(); }

  // An received thing for changed status so we can sort and filter.

  std::string         m_name;

  size_type           m_size;
  size_type           m_focus;

  // These should be replaced by a faster non-string command type.
  torrent::Object     m_sortNew;
  torrent::Object     m_sortCurrent;

  torrent::Object     m_filter;

  torrent::Object     m_event_added;
  torrent::Object     m_event_removed;

  rak::timer          m_lastChanged;

  signal_void         m_signal_changed;
  rak::priority_item  m_delayChanged;
};

}

#endif
