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

struct variable_map_data_type {
  // Some commands will need to share data, like get/set a variable. So
  // instead of using a single virtual member function, each command
  // will register a member function pointer to be used instead.
  typedef const torrent::Object (*generic_slot)(Variable*, const torrent::Object&);
  typedef const torrent::Object (*download_slot)(Variable*, core::Download*, const torrent::Object&);

  variable_map_data_type(Variable* variable, generic_slot genericSlot, download_slot downloadSlot, int flags,
                         const char* parm, const char* doc) :
    m_variable(variable), m_genericSlot(genericSlot), m_downloadSlot(downloadSlot), m_flags(flags), m_parm(parm), m_doc(doc) {}

  Variable*     m_variable;
  generic_slot  m_genericSlot;
  download_slot m_downloadSlot;

  int           m_flags;

  const char*   m_parm;
  const char*   m_doc;
};

class VariableMap : public std::map<const char*, variable_map_data_type, variable_map_comp> {
public:
  typedef std::map<const char*, variable_map_data_type, variable_map_comp> base_type;

  typedef variable_map_data_type::generic_slot  generic_slot;
  typedef variable_map_data_type::download_slot download_slot;

  typedef torrent::Object         mapped_type;
  typedef mapped_type::value_type mapped_value_type;

  using base_type::iterator;
  using base_type::key_type;
  using base_type::value_type;

  using base_type::begin;
  using base_type::end;

  static const int max_size_key = 128;
  static const int max_size_opt = 1024;
  static const int max_size_line = max_size_key + max_size_opt + 64;

  static const int flag_dont_delete   = 0x1;
  static const int flag_public_xmlrpc = 0x2;

  VariableMap() {}
  ~VariableMap();

  bool                has(const char* key) const        { return base_type::find(key) != base_type::end(); }
  bool                has(const std::string& key) const { return has(key.c_str()); }

  // Allow NULL slot as a temporary compatibility hack.

  void                insert(key_type key, Variable* variable, generic_slot genericSlot = NULL, int flags = 0,
                             const char* parm = "", const char* doc = "");

  // Consider uninlining the helper functions.
  const mapped_type   get_d(core::Download* download, key_type key) const;
  const std::string   get_d_string(core::Download* download, key_type key) const { return get_d(download, key).as_string(); }
  mapped_value_type   get_d_value(core::Download* download, key_type key) const  { return get_d(download, key).as_value(); }

  void                set_d(core::Download* download, key_type key, const mapped_type& arg);
  void                set_d_string(core::Download* download, key_type key, const std::string& arg)               { set_d(download, key, mapped_type(arg)); }
  void                set_d_std_string(core::Download* download, const std::string& key, const std::string& arg) { set_d(download, key.c_str(), mapped_type(arg)); }
  void                set_d_value(core::Download* download, key_type key, mapped_value_type arg)   { set_d(download, key, mapped_type(arg)); }

  const char*         process_single(const char* first);
  const char*         process_single(const char* first, const char* last);
  void                process_std_single(const std::string& cmd)       { process_single(cmd.c_str(), cmd.c_str() + cmd.size()); }

  const char*         process_d_single(core::Download* download, const char* first, const char* last);
  void                process_d_std_single(core::Download* download, const std::string& cmd)     { process_d_single(download, cmd.c_str(), cmd.c_str() + cmd.size()); }

  void                process_multiple(const char* first);

  void                process_stream(std::istream* str);
  bool                process_file(key_type path);

  // The new API, which is atm just a wrapper over the old and
  // requires seperate calls to get and set. These will be merged.
  const mapped_type   call_command(key_type key, const mapped_type& arg);
  const mapped_type   call_command_void(key_type key)   { return call_command(key, torrent::Object()); }
  const std::string   call_command_string(key_type key) { return call_command(key, torrent::Object()).as_string(); }
  mapped_value_type   call_command_value(key_type key)  { return call_command(key, torrent::Object()).as_value(); }

  void                call_command_set_string(key_type key, const std::string& arg)               { call_command(key, mapped_type(arg)); }
  void                call_command_set_std_string(const std::string& key, const std::string& arg) { call_command(key.c_str(), mapped_type(arg)); }

private:
  VariableMap(const VariableMap&);
  void operator = (const VariableMap&);
};

}

#endif
