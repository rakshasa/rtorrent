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
#include <rak/functional.h>
#include <torrent/exceptions.h>
#include <torrent/bencode.h>

#include "variable.h"
#include "variable_map.h"

namespace utils {

VariableMap::~VariableMap() {
  std::for_each(base_type::begin(), base_type::end(), rak::on(rak::mem_ptr_ref(&value_type::second), rak::call_delete<Variable>()));
}

void
VariableMap::insert(const std::string& key, Variable* v) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("VariableMap::insert(...) tried to insert an already existing key.");

  base_type::insert(itr, value_type(key, v));
}

const torrent::Bencode&
VariableMap::get(const std::string& key) {
  iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + key + "\" does not exist.");

  return itr->second->get();
}

void
VariableMap::set(const std::string& key, const torrent::Bencode& arg) {
  iterator itr = base_type::find(key);

  // Later, allow the user to create new variables. Have a slot to
  // register that thing.
  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + key + "\" does not exist.");

  itr->second->set(arg);
}

void
VariableMap::process_command(const std::string& command) {
  std::string::size_type pos = command.find('=');

  if (pos == std::string::npos)
    throw torrent::input_error("Option handler could not find '=' in command.");

  // Do sscanf, check for integer. Later move and make it smarter.

  set(command.substr(0, pos), torrent::Bencode(command.substr(pos + 1, std::string::npos)));
}

}
