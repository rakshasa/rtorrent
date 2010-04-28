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

namespace rpc {

// The key size should be such that the value type size which includes
// the next-pointer.

template <size_t MaxSize>
class fixed_key_type {
public:
  typedef char        value_type;
  typedef const char* iterator;
  typedef const char* const_iterator;
  typedef uint32_t    size_type;

  static const size_type max_size = MaxSize - 1;

  fixed_key_type() : m_size(0) { m_data[0] = '\0'; }
  fixed_key_type(const fixed_key_type& k) : m_size(k.m_size) { std::memcpy(m_data, k.m_data, k.m_size + 1); }

  fixed_key_type(const value_type* src_data, size_type src_size) { set_data(src_data, src_size); }

  static fixed_key_type from_c_str(const char* str)                     { fixed_key_type k; k.set_c_str(str); return k; }
  static fixed_key_type from_string(const std::string& str)             { fixed_key_type k; k.set_c_str(str.c_str(), str.size()); return k; }
  static fixed_key_type from_raw_string(const torrent::raw_string& str) { fixed_key_type k; k.set_data(str.data(), str.size()); return k; }

  bool              empty() const { return m_size == 0;; }
  size_type         size() const  { return m_size; }

  iterator          begin() const { return m_data; }
  iterator          end() const   { return m_data + m_size; }

  value_type*       data()        { return m_data; }
  const value_type* data() const  { return m_data; }
  const char*       c_str() const { return m_data; }

  void              set_data(const value_type* src_data, size_type src_size);
  void              set_c_str(const value_type* src_data);
  void              set_c_str(const value_type* src_data, size_type src_size);

  bool operator == (const fixed_key_type& rhs) const { return m_size == rhs.m_size && std::memcmp(m_data, rhs.m_data, m_size) == 0; }
  bool operator != (const fixed_key_type& rhs) const { return m_size != rhs.m_size || std::memcmp(m_data, rhs.m_data, m_size) != 0; }

  bool operator == (const std::string& rhs) const { return m_size == rhs.size() && std::memcmp(m_data, rhs.data(), m_size) == 0; }

private:
  size_type   m_size;
  char        m_data[max_size];
};

struct hash_fixed_key_type {
  template <size_t MaxSize>
  inline std::size_t operator () (const fixed_key_type<MaxSize>& p) const { return hash(p.data()); }
  
  static inline std::size_t hash(const char* data) {
    std::size_t result = 0;

    while (*data != '\0')
      result = (result * 131) + *data++;

    return result;
  }

  static inline std::size_t hash(const char* data, uint32_t size) {
    std::size_t result = 0;

    while (size--)
      result = (result * 131) + *data++;

    return result;
  }
};

//
//
//

struct object_storage_node {
  torrent::Object object;
  char            flags;
};

class object_storage : private std::tr1::unordered_map<fixed_key_type<64>, object_storage_node, hash_fixed_key_type> {
public:
  typedef std::tr1::unordered_map<fixed_key_type<64>, object_storage_node, hash_fixed_key_type> base_type;

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
  static const unsigned int flag_function_type = 0x5;

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

//   const torrent::Object& set(const torrent::raw_string& key, const torrent::Object& object);
//   const torrent::Object& set_c_str(const char* str, const torrent::Object& object) { return set(torrent::raw_string(str, std::strlen(str)), object); }

  const torrent::Object& set_bool(const torrent::raw_string& key, int64_t object);
  const torrent::Object& set_c_str_bool(const char* str, int64_t object) { return set_bool(torrent::raw_string::from_c_str(str), object); }
  const torrent::Object& set_str_bool(const std::string& str, int64_t object) { return set_bool(torrent::raw_string::from_string(str), object); }

  const torrent::Object& set_value(const torrent::raw_string& key, int64_t object);
  const torrent::Object& set_c_str_value(const char* str, int64_t object) { return set_value(torrent::raw_string::from_string(str), object); }
  const torrent::Object& set_str_value(const std::string& str, int64_t object) { return set_value(torrent::raw_string::from_string(str), object); }

  const torrent::Object& set_string(const torrent::raw_string& key, const std::string& object);
  const torrent::Object& set_c_str_string(const char* str, const std::string& object) { return set_string(torrent::raw_string::from_c_str(str), object); }
  const torrent::Object& set_str_string(const std::string& str, const std::string& object) { return set_string(torrent::raw_string::from_string(str), object); }
  
  torrent::Object call_function(const torrent::raw_string& key, target_type target, const torrent::Object& object);
  torrent::Object call_function_str(const std::string& key, target_type target, const torrent::Object& object);

  const torrent::Object& set_function(const torrent::raw_string& key, const std::string& object);
  const torrent::Object& set_str_function(const std::string& str, const std::string& object) { return set_function(torrent::raw_string::from_string(str), object); }
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

}

#endif
