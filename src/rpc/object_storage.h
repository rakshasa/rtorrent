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
#include <tr1/unordered_map>
#include <torrent/object.h>

#include "command.h"
#include "fixed_key.h"

namespace rpc {

struct object_storage_node {
  torrent::Object object;
  char            flags;
};

class object_storage : private std::unordered_map<fixed_key_type<64>, object_storage_node, hash_fixed_key_type> {
public:
  typedef std::unordered_map<fixed_key_type<64>, object_storage_node, hash_fixed_key_type> base_type;

  using base_type::key_type;
  using base_type::value_type;
  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::local_iterator;
  using base_type::const_local_iterator;

  using base_type::begin;
  using base_type::end;
  using base_type::size;
  using base_type::empty;
  using base_type::key_eq;
  using base_type::bucket;
  using base_type::bucket_count;
  using base_type::max_bucket_count;
  using base_type::load_factor;

  using base_type::clear;
  using base_type::find;
  using base_type::erase;
  
  static const unsigned int flag_generic_type  = 0x1;
  static const unsigned int flag_bool_type     = 0x2;
  static const unsigned int flag_value_type    = 0x3;
  static const unsigned int flag_string_type   = 0x4;
  static const unsigned int flag_list_type     = 0x5;
  static const unsigned int flag_function_type = 0x6;
  static const unsigned int flag_multi_type    = 0x7;

  static const unsigned int mask_type          = 0xf;

  static const unsigned int flag_constant = 0x10;
  static const unsigned int flag_static   = 0x20;
  static const unsigned int flag_private  = 0x40;

  static const size_t key_size = key_type::max_size;

  local_iterator find_local(const torrent::raw_string& key);

  iterator insert(const char* key_data, uint32_t key_size, const torrent::Object& object, unsigned int flags);
  iterator insert_c_str(const char* key, const torrent::Object& object, unsigned int flags) { return insert(key, std::strlen(key), object, flags); }

  iterator insert(const char* key, const torrent::Object& object, unsigned int flags);
  iterator insert(const torrent::raw_string& key, const torrent::Object& object, unsigned int flags);
  iterator insert_str(const std::string& key, const torrent::Object& object, unsigned int flags);

  // Access functions that throw on error.

  const torrent::Object& get(const torrent::raw_string& key);
  const torrent::Object& get_c_str(const char* str)  { return get(torrent::raw_string(str, std::strlen(str))); }
  const torrent::Object& get_str(const std::string& str) { return get(torrent::raw_string(str.data(), str.size())); }

  const torrent::Object& set_bool(const torrent::raw_string& key, int64_t object);
  const torrent::Object& set_c_str_bool(const char* str, int64_t object) { return set_bool(torrent::raw_string::from_c_str(str), object); }
  const torrent::Object& set_str_bool(const std::string& str, int64_t object) { return set_bool(torrent::raw_string::from_string(str), object); }

  const torrent::Object& set_value(const torrent::raw_string& key, int64_t object);
  const torrent::Object& set_c_str_value(const char* str, int64_t object) { return set_value(torrent::raw_string::from_string(str), object); }
  const torrent::Object& set_str_value(const std::string& str, int64_t object) { return set_value(torrent::raw_string::from_string(str), object); }

  const torrent::Object& set_string(const torrent::raw_string& key, const std::string& object);
  const torrent::Object& set_c_str_string(const char* str, const std::string& object) { return set_string(torrent::raw_string::from_c_str(str), object); }
  const torrent::Object& set_str_string(const std::string& str, const std::string& object) { return set_string(torrent::raw_string::from_string(str), object); }
  
  const torrent::Object& set_list(const torrent::raw_string& key, const torrent::Object::list_type& object);
  const torrent::Object& set_c_str_list(const char* str, const torrent::Object::list_type& object) { return set_list(torrent::raw_string::from_c_str(str), object); }
  const torrent::Object& set_str_list(const std::string& str, const torrent::Object::list_type& object) { return set_list(torrent::raw_string::from_string(str), object); }

  // Functions callers:
  torrent::Object call_function(const torrent::raw_string& key, target_type target, const torrent::Object& object);
  torrent::Object call_function_str(const std::string& key, target_type target, const torrent::Object& object);

  // Single-command function:

  const torrent::Object& set_function(const torrent::raw_string& key, const std::string& object);
  const torrent::Object& set_str_function(const std::string& key, const std::string& object);

  // Multi-command function:
  bool                   has_multi_key(const torrent::raw_string& key, const std::string& cmd_key);
  void                   erase_multi_key(const torrent::raw_string& key, const std::string& cmd_key);
  void                   set_multi_key(const torrent::raw_string& key, const std::string& cmd_key, const std::string& object);

  bool                   has_str_multi_key(const std::string& key, const std::string& cmd_key);
  void                   erase_str_multi_key(const std::string& key, const std::string& cmd_key);
  void                   set_str_multi_key(const std::string& key, const std::string& cmd_key, const std::string& object);
};

//
// Implementation:
//

template <size_t MaxSize> inline void
fixed_key_type<MaxSize>::set_data(const value_type* src_data, size_type src_size) {
  if (src_size >= max_size) {
    new (this) fixed_key_type();
    return;
  }

  m_size = src_size;
  std::memcpy(m_data, src_data, m_size);
  m_data[m_size] = '\0';
}

template <size_t MaxSize> inline void
fixed_key_type<MaxSize>::set_c_str(const value_type* src_data) {
  value_type* itr = m_data;
  const value_type* last = m_data + max_size;

  while (itr != last && *src_data != '\0')
    *itr++ = *src_data++;

  *itr = '\0';
  m_size = std::distance(m_data, itr);
}

template <size_t MaxSize> inline void
fixed_key_type<MaxSize>::set_c_str(const value_type* src_data, size_type src_size) {
  if (src_size >= max_size) {
    new (this) fixed_key_type();
    return;
  }

  m_size = src_size;
  std::memcpy(m_data, src_data, m_size + 1);
}

inline object_storage::iterator
object_storage::insert(const char* key, const torrent::Object& object, unsigned int flags) {
  return insert(key, std::strlen(key), object, flags);
}

inline object_storage::iterator
object_storage::insert(const torrent::raw_string& key, const torrent::Object& object, unsigned int flags) {
  return insert(key.data(), key.size(), object, flags);
}

inline object_storage::iterator
object_storage::insert_str(const std::string& key, const torrent::Object& object, unsigned int flags) {
  return insert(key.data(), key.size(), object, flags);
}

inline torrent::Object
object_storage::call_function_str(const std::string& key, target_type target, const torrent::Object& object) {
  return call_function(torrent::raw_string::from_string(key), target, object);
}

inline const torrent::Object&
object_storage::set_str_function(const std::string& key, const std::string& object) {
  return set_function(torrent::raw_string::from_string(key), object);
}

inline bool
object_storage::has_str_multi_key(const std::string& key, const std::string& cmd_key) {
  return has_multi_key(torrent::raw_string::from_string(key), cmd_key);
}

inline void
object_storage::erase_str_multi_key(const std::string& key, const std::string& cmd_key) {
  erase_multi_key(torrent::raw_string::from_string(key), cmd_key);
}

inline void
object_storage::set_str_multi_key(const std::string& key, const std::string& cmd_key, const std::string& object) {
  return set_multi_key(torrent::raw_string::from_string(key), cmd_key, object);
}

}

#endif
