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

#ifndef RTORRENT_UTILS_VARIABLE_MAP_H
#define RTORRENT_UTILS_VARIABLE_MAP_H

#include <map>
#include <string>
#include <cstring>
#include <iosfwd>
#include <torrent/object.h>

namespace core {
  class Download;
}

namespace torrent {
  class Object;
}

namespace utils {

struct variable_map_comp : public std::binary_function<const char*, const char*, bool> {
  bool operator () (const char* arg1, const char* arg2) const { return std::strcmp(arg1, arg2) < 0; }
};

class Variable;

class VariableMap : public std::map<const char*, Variable*, variable_map_comp> {
public:
  typedef std::map<const char*, Variable*, variable_map_comp> base_type;

  typedef torrent::Object         mapped_type;
  typedef mapped_type::value_type mapped_value_type;

  static const int max_size_key = 128;
  static const int max_size_opt = 1024;
  static const int max_size_line = max_size_key + max_size_opt + 64;

  using base_type::iterator;
  using base_type::key_type;
  using base_type::value_type;

  VariableMap() {}
  ~VariableMap();

  bool                has(const char* key) const        { return base_type::find(key) != base_type::end(); }
  bool                has(const std::string& key) const { return has(key.c_str()); }

  void                insert(key_type key, Variable* v);

  // Consider uninlining the helper functions.

  const mapped_type&  get(key_type key) const;
  const mapped_type&  get_d(core::Download* download, key_type key) const;

  const std::string&  get_string(key_type key) const                             { return get(key).as_string(); }
  const std::string&  get_d_string(core::Download* download, key_type key) const { return get_d(download, key).as_string(); }

  mapped_value_type   get_value(key_type key) const                              { return get(key).as_value(); }
  mapped_value_type   get_d_value(core::Download* download, key_type key) const  { return get_d(download, key).as_value(); }

  void                set(key_type key, const mapped_type& arg);
  void                set_d(core::Download* download, key_type key, const mapped_type& arg);

  void                set_string(key_type key, const std::string& arg)                                           { set(key, mapped_type(arg)); }
  void                set_d_string(core::Download* download, key_type key, const std::string& arg)               { set_d(download, key, mapped_type(arg)); }
  void                set_d_std_string(core::Download* download, const std::string& key, const std::string& arg) { set_d(download, key.c_str(), mapped_type(arg)); }

  void                set_value(key_type key, mapped_value_type arg)                               { set(key, mapped_type(arg)); }
  void                set_d_value(core::Download* download, key_type key, mapped_value_type arg)   { set_d(download, key, mapped_type(arg)); }

  void                set_std_string(const std::string& key, const std::string& arg) { set(key.c_str(), mapped_type(arg)); }

  const char*         process_single(const char* first);
  const char*         process_single(const char* first, const char* last);
  void                process_std_single(const std::string& cmd)       { process_single(cmd.c_str(), cmd.c_str() + cmd.size()); }

  const char*         process_d_single(core::Download* download, const char* first, const char* last);
  void                process_d_std_single(core::Download* download, const std::string& cmd)     { process_d_single(download, cmd.c_str(), cmd.c_str() + cmd.size()); }

  void                process_multiple(const char* first);

  void                process_stream(std::istream* str);
  bool                process_file(key_type path);

private:
  VariableMap(const VariableMap&);
  void operator = (const VariableMap&);
};

}

#endif
