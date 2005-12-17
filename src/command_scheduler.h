// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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

#ifndef RTORRENT_COMMAND_SCHEDULER_H
#define RTORRENT_COMMAND_SCHEDULER_H

#include <vector>
#include <string>
#include <rak/functional_fun.h>

class CommandSchedulerItem;

class CommandScheduler : public std::vector<CommandSchedulerItem*> {
public:
  typedef rak::function1<void, const std::string&> SlotString;
  typedef std::vector<CommandSchedulerItem*>       base_type;

  using base_type::value_type;
  using base_type::begin;
  using base_type::end;

  CommandScheduler() {}
  ~CommandScheduler();

  void                set_slot_command(SlotString::base_type* s)       { m_slotCommand.set(s); }
  void                set_slot_error_message(SlotString::base_type* s) { m_slotErrorMessage.set(s); }

  // slot_error_message or something.

  iterator            find(const std::string& key);

  // If the key already exists then the old item is deleted. It is
  // safe to call erase on end().
  iterator            insert(const std::string& key);
  void                erase(iterator itr);

private:
  void                call_item(value_type item);

  SlotString          m_slotCommand;
  SlotString          m_slotErrorMessage;
};

#endif
