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

#ifndef RTORRENT_COMMAND_SCHEDULER_ITEM_H
#define RTORRENT_COMMAND_SCHEDULER_ITEM_H

#include "globals.h"

#include lt_tr1_functional
#include <torrent/object.h>

namespace rpc {

class CommandSchedulerItem {
public:
  typedef std::function<void ()> slot_void;

  CommandSchedulerItem(const std::string& key) : m_key(key), m_interval(0) {}
  ~CommandSchedulerItem();

  bool                is_queued() const           { return m_task.is_queued(); }

  void                enable(rak::timer t);
  void                disable();

  const std::string&  key() const                 { return m_key; }
  torrent::Object&    command()                   { return m_command; }

  // 'interval()' should in the future return some more dynamic values.
  uint32_t            interval() const            { return m_interval; }
  void                set_interval(uint32_t v)    { m_interval = v; }

  rak::timer          time_scheduled() const      { return m_timeScheduled; }
  rak::timer          next_time_scheduled() const;

  slot_void&          slot()                      { return m_task.slot(); }

private:
  CommandSchedulerItem(const CommandSchedulerItem&);
  void operator = (const CommandSchedulerItem&);

  std::string         m_key;
  torrent::Object     m_command;
  
  uint32_t            m_interval;
  rak::timer          m_timeScheduled;

  rak::priority_item  m_task;

  // Flags for various things.
};

}

#endif
