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

// Provides a filtered and sorted list of downloads that can be
// updated auto-magically.
//
// We don't worry about std::vector's insert/erase performance as it
// get's called so often, better with cache locality.
//
// Do we want to be able to modify the underlying DownloadList from
// here?

#ifndef RTORRENT_CORE_VIEW_DOWNLOADS_H
#define RTORRENT_CORE_VIEW_DOWNLOADS_H

#include <string>
#include <vector>
#include <sigc++/signal.h>

namespace core {

class Download;
class DownloadList;

class ViewDownloads : public std::vector<core::Download*> {
public:
  typedef std::vector<core::Download*> base_type;
  typedef sigc::signal0<void>          signal_type;

  // Switch with a base class that defines some function. We might
  // want to include some parameters etc.
  //
  // Also, it should be possible to check the focus index etc.
  typedef bool (*sort_slot)(core::Download* d1, core::Download* d2);

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

  ViewDownloads() : m_sortNew(NULL) {}
  ~ViewDownloads();

  void                initialize(const std::string& name, core::DownloadList* list);

  const std::string&  name() const                            { return m_name; }

  iterator            focus()                                 { return begin() + m_focus; }
  const_iterator      focus() const                           { return begin() + m_focus; }
  void                set_focus(iterator itr)                 { m_focus = position(itr); m_signalChanged.emit(); }

  void                next_focus();
  void                prev_focus();

  void                sort(sort_slot s);

  void                set_sort_new(sort_slot s)               { m_sortNew = s; }

  signal_type&        signal_changed()                        { return m_signalChanged; }

private:
  ViewDownloads(const ViewDownloads&);
  void operator = (const ViewDownloads&);

  void                received_insert(core::Download* d);
  void                received_erase(core::Download* d);

  size_type           position(const_iterator itr) const      { return (size_type)(itr - begin()); }

  // An received thing for changed status so we can sort and filter.

  std::string         m_name;

  core::DownloadList* m_list;
  size_type           m_focus;

  sort_slot           m_sortNew;

  // Timer, last changed.

  signal_type         m_signalChanged;
};

}

#endif
