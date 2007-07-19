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

#ifndef RTORRENT_RPC_COMMAND_MAP_H
#define RTORRENT_RPC_COMMAND_MAP_H

#include <map>
#include <string>
#include <cstring>
#include <torrent/object.h>

namespace core {
  class Download;
}

namespace torrent {
  class File;
}

namespace rpc {

class Command;

struct command_map_comp : public std::binary_function<const char*, const char*, bool> {
  bool operator () (const char* arg1, const char* arg2) const { return std::strcmp(arg1, arg2) < 0; }
};

struct command_map_data_type {
  // Some commands will need to share data, like get/set a variable. So
  // instead of using a single virtual member function, each command
  // will register a member function pointer to be used instead.
  typedef const torrent::Object (*generic_slot)(Command*, const torrent::Object&);
  typedef const torrent::Object (*download_slot)(Command*, core::Download*, const torrent::Object&);
  typedef const torrent::Object (*file_slot)(Command*, torrent::File*, const torrent::Object&);

  command_map_data_type(Command* variable, int flags, const char* parm, const char* doc) :
    m_variable(variable), m_genericSlot(NULL), m_downloadSlot(NULL), m_fileSlot(NULL),
    m_flags(flags), m_parm(parm), m_doc(doc) {}

  Command*      m_variable;

  // Should make this into a union and pass a type id when calling
  // commands, making it all use the same generic interface.
  generic_slot  m_genericSlot;
  download_slot m_downloadSlot;
  file_slot     m_fileSlot;

  int           m_flags;

  const char*   m_parm;
  const char*   m_doc;
};

class CommandMap : public std::map<const char*, command_map_data_type, command_map_comp> {
public:
  typedef std::map<const char*, command_map_data_type, command_map_comp> base_type;

  typedef command_map_data_type::generic_slot  generic_slot;
  typedef command_map_data_type::download_slot download_slot;
  typedef command_map_data_type::file_slot     file_slot;

  typedef torrent::Object         mapped_type;
  typedef mapped_type::value_type mapped_value_type;

  using base_type::iterator;
  using base_type::key_type;
  using base_type::value_type;

  using base_type::begin;
  using base_type::end;
  using base_type::find;

  static const int flag_dont_delete   = 0x1;
  static const int flag_public_xmlrpc = 0x2;

  CommandMap() {}
  ~CommandMap();

  bool                has(const char* key) const        { return base_type::find(key) != base_type::end(); }
  bool                has(const std::string& key) const { return has(key.c_str()); }

  iterator            insert(key_type key, Command* variable, int flags, const char* parm, const char* doc);

  void                insert(key_type key, Command* variable, generic_slot genericSlot, download_slot downloadSlot, int flags,
                             const char* parm, const char* doc);

  void                insert_file(key_type key, Command* variable, file_slot fileSlot, int flags, const char* parm, const char* doc);

  void                insert(key_type key, const command_map_data_type src);

  const mapped_type   call_command(key_type key, const mapped_type& arg);
  const mapped_type   call_command_d(key_type key, core::Download* download, const mapped_type& arg);
  const mapped_type   call_command_f(key_type key, torrent::File* file, const mapped_type& arg);

private:
  CommandMap(const CommandMap&);
  void operator = (const CommandMap&);
};

}

#endif
