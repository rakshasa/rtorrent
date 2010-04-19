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

#include "object_storage.h"

#include "parse.h"

namespace rpc {

object_storage::local_iterator
object_storage::find_local(const torrent::raw_string& key) {
  std::size_t n = hash_fixed_key_type::hash(key.data(), key.size()) % bucket_count();

  for (local_iterator itr = begin(n), last = end(n); itr != last; itr++)
    if (itr->first.size() == key.size() && std::memcmp(itr->first.data(), key.data(), key.size()) == 0)
      return itr;

  return end(bucket_count());
}

object_storage::iterator
object_storage::insert(const char* key_data, uint32_t key_size, const torrent::Object& object, unsigned int flags) {
  if (std::find(key_data, key_data + key_size, '\0') != key_data + key_size)
    throw torrent::input_error("Found nul-char in string.");

  // Check for size > key_size.
  // Check for empty string.

  // Ensure the object type is correct.

  if (!(flags & mask_type))
    throw torrent::input_error("No type flags set when calling object_storage::insert.");

  std::pair<iterator, bool> result = base_type::insert(std::make_pair(key_type(key_data, key_size), object_storage_node()));

  if (!result.second)
    throw torrent::input_error("Key already exists in object_storage.");

  result.first->second.object = object;
  result.first->second.flags = flags;

  return result.first;
}

const torrent::Object&
object_storage::get(const torrent::raw_string& key) {
  local_iterator itr = find_local(key);

  if (itr == end(bucket_count()))
    throw torrent::input_error("Key not found.");

  return itr->second.object;
}

const torrent::Object&
object_storage::set(const torrent::raw_string& key, const torrent::Object& object) {
  local_iterator itr = find_local(key);

  if (itr == end(bucket_count()))
    throw torrent::input_error("Key not found.");

  // Redo this...

  switch (itr->second.flags & mask_type) {
  case flag_generic_type: itr->second.object = object; break;
  case flag_value_type:  itr->second.object = convert_to_value(object); break;
  case flag_string_type: itr->second.object = convert_to_string(object); break;
  default: throw torrent::internal_error("object_storage::set: Type not set.");
  }

  return itr->second.object;
}

}
