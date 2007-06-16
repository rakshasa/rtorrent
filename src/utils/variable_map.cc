// rTorrent - BitTorrent client
// Copyright (C) 2006, Jari Sundell
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
#include <cstdio>
#include <cctype>
#include <fstream>
#include <rak/functional.h>
#include <rak/path.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>

#include "rpc/command.h"
#include "variable_map.h"

namespace utils {

struct variable_map_get_ptr : std::unary_function<VariableMap::value_type&, Command*> {
  Command* operator () (VariableMap::value_type& value) { return value.second.m_variable; }
};

VariableMap::~VariableMap() {
  for (iterator itr = base_type::begin(), last = base_type::end(); itr != last; itr++)
    if (!(itr->second.m_flags & flag_dont_delete))
      delete itr->second.m_variable;
}

void
VariableMap::insert(key_type key, Command* variable, generic_slot genericSlot, download_slot downloadSlot, int flags,
                    const char* parm, const char* doc) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("VariableMap::insert(...) tried to insert an already existing key.");

  base_type::insert(itr, value_type(key, variable_map_data_type(variable, genericSlot, downloadSlot, flags, parm, doc)));
}

void
VariableMap::insert(key_type key, const variable_map_data_type src) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("VariableMap::insert(...) tried to insert an already existing key.");

  base_type::insert(itr, value_type(key, variable_map_data_type(src.m_variable, src.m_genericSlot, src.m_downloadSlot,
                                                                src.m_flags | flag_dont_delete, src.m_parm, src.m_doc)));
}

const VariableMap::mapped_type
VariableMap::call_command(key_type key, const mapped_type& arg) {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Command \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_genericSlot == NULL)
    throw torrent::input_error("Command does not have a generic slot.");

  return itr->second.m_genericSlot(itr->second.m_variable, arg);
}

const VariableMap::mapped_type
VariableMap::call_command_d(key_type key, core::Download* download, const mapped_type& arg) {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Command \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_downloadSlot == NULL) {
    if (itr->second.m_genericSlot == NULL)
      throw torrent::input_error("Command does not have a generic slot.");

    return itr->second.m_genericSlot(itr->second.m_variable, arg);
  }

  return itr->second.m_downloadSlot(itr->second.m_variable, download, arg);
}

}
