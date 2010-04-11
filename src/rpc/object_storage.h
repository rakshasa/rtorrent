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

// The object_storage type is responsible for storing variables,
// commands, command lists and other types encoded in torrent::Object
// format.

#ifndef RTORRENT_RPC_OBJECT_STORAGE_H
#define RTORRENT_RPC_OBJECT_STORAGE_H

#include <cstring>
#include <tr1/unordered_set>
#include <torrent/object.h>

namespace rpc {

// The key size should be such that the value type size which includes
// the next-pointer.

struct object_storage_node_internal {
  torrent::Object object;
  char            key[0];
};

struct object_storage_node : public object_storage_node_internal {
  // TODO: Use the size of the unordered_map::value_type with a struct {}.
  static const size_t key_size = 128 - sizeof(object_storage_node_internal) - sizeof(void*);
};

struct hash_object_storage_node {
  inline std::size_t operator () (const object_storage_node& p);
};

bool operator == (const object_storage_node& left, const object_storage_node& right) { return std::strcmp(left.key, right.key) == 0; }
bool operator != (const object_storage_node& left, const object_storage_node& right) { return std::strcmp(left.key, right.key) != 0; }
bool operator <  (const object_storage_node& left, const object_storage_node& right) { return std::strcmp(left.key, right.key) <  0; }
bool operator <= (const object_storage_node& left, const object_storage_node& right) { return std::strcmp(left.key, right.key) <= 0; }
bool operator >  (const object_storage_node& left, const object_storage_node& right) { return std::strcmp(left.key, right.key) >  0; }
bool operator >= (const object_storage_node& left, const object_storage_node& right) { return std::strcmp(left.key, right.key) >= 0; }

class object_storage : private std::tr1::unordered_set<object_storage_node, hash_object_storage_node> {
  typedef std::tr1::unordered_set<object_storage_node, hash_object_storage_node> base_type;

  using base_type::iterator;
};

inline std::size_t
hash_object_storage_node::operator () (const object_storage_node& p) {
  std::size_t result = 0;
  const char* first = p.key;

  while (*first != '\0')
    result = (result * 131) + *first++;

  return result;
}

}

#endif
