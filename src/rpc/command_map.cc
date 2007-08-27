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
CommandMap::insert_generic(key_type key, Command* variable, generic_slot targetSlot, int flags, const char* parm, const char* doc) {
  iterator itr = insert(key, variable, flags, parm, doc);

  itr->second.m_target      = target_generic;
  itr->second.m_genericSlot = targetSlot;
}

void
CommandMap::insert_download(key_type key, Command* variable, download_slot targetSlot, int flags, const char* parm, const char* doc) {
  iterator itr = insert(key, variable, flags, parm, doc);

  itr->second.m_target       = target_download;
  itr->second.m_downloadSlot = targetSlot;
}

void
CommandMap::insert_file(key_type key, Command* variable, file_slot targetSlot, int flags, const char* parm, const char* doc) {
  iterator itr = insert(key, variable, flags, parm, doc);

  itr->second.m_target   = target_file;
  itr->second.m_fileSlot = targetSlot;
}

void
CommandMap::insert_peer(key_type key, Command* variable, peer_slot targetSlot, int flags, const char* parm, const char* doc) {
  iterator itr = insert(key, variable, flags, parm, doc);

  itr->second.m_target   = target_peer;
  itr->second.m_peerSlot = targetSlot;
}

void
CommandMap::insert_tracker(key_type key, Command* variable, tracker_slot targetSlot, int flags, const char* parm, const char* doc) {
  iterator itr = insert(key, variable, flags, parm, doc);

  itr->second.m_target      = target_tracker;
  itr->second.m_trackerSlot = targetSlot;
}

void
CommandMap::insert(key_type key, const command_map_data_type src) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("CommandMap::insert(...) tried to insert an already existing key.");

  itr = base_type::insert(itr, value_type(key, command_map_data_type(src.m_variable, src.m_flags | flag_dont_delete, src.m_parm, src.m_doc)));

  itr->second.m_target       = src.m_target;

  // This _should_ be optimized int just one assignment.
  switch (itr->second.m_target) {
  case target_generic:  itr->second.m_genericSlot  = src.m_genericSlot; break;
  case target_download: itr->second.m_downloadSlot = src.m_downloadSlot; break;
  case target_file:     itr->second.m_fileSlot     = src.m_fileSlot; break;
  case target_peer:     itr->second.m_peerSlot     = src.m_peerSlot; break;
  case target_tracker:  itr->second.m_trackerSlot  = src.m_trackerSlot; break;
  default: throw torrent::internal_error("CommandMap::insert(...) Invalid target.");
  }
}

const CommandMap::mapped_type
CommandMap::call_command(key_type key, const mapped_type& arg, target_type target) {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Command \"" + std::string(key) + "\" does not exist.");

  if ((itr->second.m_target != target.first && itr->second.m_target != target_generic) ||
      (itr->second.m_target != target_generic && target.second == NULL))
    throw torrent::input_error("Command type mis-match.");

  // This _should_ be optimized int just two calls.
  switch (itr->second.m_target) {
  case target_generic:  return itr->second.m_genericSlot(itr->second.m_variable, arg);
  case target_download: return itr->second.m_downloadSlot(itr->second.m_variable, (core::Download*)target.second, arg);
  case target_file:     return itr->second.m_fileSlot(itr->second.m_variable, (torrent::File*)target.second, arg);
  case target_peer:     return itr->second.m_peerSlot(itr->second.m_variable, (torrent::Peer*)target.second, arg);
  case target_tracker:  return itr->second.m_trackerSlot(itr->second.m_variable, (torrent::Tracker*)target.second, arg);
  default: throw torrent::internal_error("CommandMap::call_command(...) Invalid target.");
  }
}

const CommandMap::mapped_type
CommandMap::call_command(const_iterator itr, const mapped_type& arg, target_type target) {
  if ((itr->second.m_target != target.first && itr->second.m_target != target_generic) ||
      (itr->second.m_target != target_generic && target.second == NULL))
    throw torrent::input_error("Command type mis-match.");

  // This _should_ be optimized int just two calls.
  switch (itr->second.m_target) {
  case target_generic:  return itr->second.m_genericSlot(itr->second.m_variable, arg);
  case target_download: return itr->second.m_downloadSlot(itr->second.m_variable, (core::Download*)target.second, arg);
  case target_file:     return itr->second.m_fileSlot(itr->second.m_variable, (torrent::File*)target.second, arg);
  case target_peer:     return itr->second.m_peerSlot(itr->second.m_variable, (torrent::Peer*)target.second, arg);
  case target_tracker:  return itr->second.m_trackerSlot(itr->second.m_variable, (torrent::Tracker*)target.second, arg);
  default: throw torrent::internal_error("CommandMap::call_command(...) Invalid target.");
  }
}

}
