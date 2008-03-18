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

#include <torrent/exceptions.h>
#include <torrent/object.h>
#include <torrent/data/file_list_iterator.h>

#include "command.h"
#include "command_map.h"

namespace rpc {

CommandMap::~CommandMap() {
  for (iterator itr = base_type::begin(), last = base_type::end(); itr != last; itr++)
    if (!(itr->second.m_flags & flag_dont_delete))
      delete itr->second.m_variable;
}

CommandMap::iterator
CommandMap::insert(key_type key, Command* variable, int flags, const char* parm, const char* doc) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("CommandMap::insert(...) tried to insert an already existing key.");

  return base_type::insert(itr, value_type(key, command_map_data_type(variable, flags, parm, doc)));
}

void
CommandMap::insert(key_type key, const command_map_data_type src) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("CommandMap::insert(...) tried to insert an already existing key.");

  itr = base_type::insert(itr, value_type(key, command_map_data_type(src.m_variable, src.m_flags | flag_dont_delete, src.m_parm, src.m_doc)));

  // We can assume all the slots are the same size.
  itr->second.m_target      = src.m_target;
  itr->second.m_genericSlot = src.m_genericSlot;
}

const CommandMap::mapped_type
CommandMap::call_command(key_type key, const mapped_type& arg, target_type target) {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Command \"" + std::string(key) + "\" does not exist.");

  if (target.first != Command::target_generic && target.second == NULL) {
    // We received a target that is NULL, so throw an exception unless
    // we can convert it to a void target.
    if (itr->second.m_target > Command::target_any)
      throw torrent::input_error("Command type mis-match.");

    target.first = Command::target_generic;
  }

  if (itr->second.m_target != target.first && itr->second.m_target > Command::target_any) {
    // Mismatch between the target and command type. If it is not
    // possible to convert, then throw an input error.
    if (target.first == Command::target_file_itr && itr->second.m_target == Command::target_file)
      target = target_type((int)Command::target_file, static_cast<torrent::FileListIterator*>(target.second)->file());
    else
      throw torrent::input_error("Command type mis-match.");
  }

  // This _should_ be optimized int just two calls.
  switch (itr->second.m_target) {
  case Command::target_generic:  return itr->second.m_genericSlot (itr->second.m_variable, arg);
  case Command::target_any:      return itr->second.m_anySlot     (itr->second.m_variable, target, arg);
  case Command::target_download: return itr->second.m_downloadSlot(itr->second.m_variable, (core::Download*)target.second, arg);
  case Command::target_peer:     return itr->second.m_peerSlot    (itr->second.m_variable, (torrent::Peer*)target.second, arg);
  case Command::target_tracker:  return itr->second.m_trackerSlot (itr->second.m_variable, (torrent::Tracker*)target.second, arg);
  case Command::target_file:     return itr->second.m_fileSlot    (itr->second.m_variable, (torrent::File*)target.second, arg);
  case Command::target_file_itr: return itr->second.m_fileItrSlot (itr->second.m_variable, (torrent::FileListIterator*)target.second, arg);

  case Command::target_download_pair: return itr->second.m_downloadPairSlot(itr->second.m_variable, (core::Download*)target.second, (core::Download*)target.third, arg);

  default: throw torrent::internal_error("CommandMap::call_command(...) Invalid target.");
  }
}

const CommandMap::mapped_type
CommandMap::call_command(const_iterator itr, const mapped_type& arg, target_type target) {
  if (target.first != Command::target_generic && target.second == NULL) {
    // We received a target that is NULL, so throw an exception unless
    // we can convert it to a void target.
    if (itr->second.m_target > Command::target_any)
      throw torrent::input_error("Command type mis-match.");

    target.first = Command::target_generic;
  }

  if (itr->second.m_target != target.first && itr->second.m_target > Command::target_any)
    throw torrent::input_error("Command type mis-match.");

  // This _should_ be optimized int just two calls.
  switch (itr->second.m_target) {
  case Command::target_generic:  return itr->second.m_genericSlot (itr->second.m_variable, arg);
  case Command::target_any:      return itr->second.m_anySlot     (itr->second.m_variable, target, arg);
  case Command::target_download: return itr->second.m_downloadSlot(itr->second.m_variable, (core::Download*)target.second, arg);
  case Command::target_peer:     return itr->second.m_peerSlot    (itr->second.m_variable, (torrent::Peer*)target.second, arg);
  case Command::target_tracker:  return itr->second.m_trackerSlot (itr->second.m_variable, (torrent::Tracker*)target.second, arg);
  case Command::target_file:     return itr->second.m_fileSlot    (itr->second.m_variable, (torrent::File*)target.second, arg);
  case Command::target_file_itr: return itr->second.m_fileItrSlot (itr->second.m_variable, (torrent::FileListIterator*)target.second, arg);

  case Command::target_download_pair: return itr->second.m_downloadPairSlot(itr->second.m_variable, (core::Download*)target.second, (core::Download*)target.third, arg);

  default: throw torrent::internal_error("CommandMap::call_command(...) Invalid target.");
  }
}

}
