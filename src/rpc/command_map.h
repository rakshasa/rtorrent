// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include "command.h"

namespace rpc {

struct command_map_comp : public std::binary_function<const char*, const char*, bool> {
  bool operator () (const char* arg1, const char* arg2) const { return std::strcmp(arg1, arg2) < 0; }
};

struct command_map_data_type {
  // Some commands will need to share data, like get/set a variable. So
  // instead of using a single virtual member function, each command
  // will register a member function pointer to be used instead.
  //
  // The any_slot should perhaps replace generic_slot?

  command_map_data_type(int flags, const char* parm, const char* doc) :
    m_flags(flags), m_parm(parm), m_doc(doc) {}

  command_map_data_type(const command_map_data_type& src) :
    m_variable(src.m_variable), m_anySlot(src.m_anySlot),
    m_flags(src.m_flags), m_parm(src.m_parm), m_doc(src.m_doc) {}
  
  command_base             m_variable;
  command_base::any_slot   m_anySlot;

  int           m_flags;

  const char*   m_parm;
  const char*   m_doc;
};

class CommandMap : public std::map<const char*, command_map_data_type, command_map_comp> {
public:
  typedef std::map<const char*, command_map_data_type, command_map_comp> base_type;

  typedef torrent::Object         mapped_type;
  typedef mapped_type::value_type mapped_value_type;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::key_type;
  using base_type::value_type;

  using base_type::begin;
  using base_type::end;
  using base_type::find;

  static const int flag_dont_delete   = 0x1;
  static const int flag_delete_key    = 0x2;
  static const int flag_public_xmlrpc = 0x4;
  static const int flag_modifiable    = 0x10;
  static const int flag_is_redirect   = 0x20;
  static const int flag_has_redirects = 0x40;

  static const int flag_no_target      = 0x100;
  static const int flag_file_target    = 0x200;
  static const int flag_tracker_target = 0x400;

  CommandMap() {}
  ~CommandMap();

  bool                has(const char* key) const        { return base_type::find(key) != base_type::end(); }
  bool                has(const std::string& key) const { return has(key.c_str()); }

  bool                is_modifiable(const_iterator itr) { return itr != end() && (itr->second.m_flags & flag_modifiable); }

  iterator            insert(key_type key, int flags, const char* parm, const char* doc);

  template <typename T, typename Slot>
  void
  insert_slot(key_type key, Slot variable, command_base::any_slot targetSlot, int flags, const char* parm, const char* doc) {
    iterator itr = insert(key, flags, parm, doc);
    itr->second.m_variable.set_function<T>(variable);
    itr->second.m_anySlot = targetSlot;
  }

  //  void                insert(key_type key, const command_map_data_type src);
  void                erase(iterator itr);

  void                create_redirect(key_type key_new, key_type key_dest, int flags);

  const mapped_type   call(key_type key, const mapped_type& args = mapped_type());
  const mapped_type   call(key_type key, target_type target, const mapped_type& args = mapped_type()) { return call_command(key, args, target); }
  const mapped_type   call_catch(key_type key, target_type target, const mapped_type& args = mapped_type(), const char* err = "Command failed: ");

  const mapped_type   call_command  (key_type key, const mapped_type& arg, target_type target = target_type((int)command_base::target_generic, NULL));
  const mapped_type   call_command  (iterator itr, const mapped_type& arg, target_type target = target_type((int)command_base::target_generic, NULL));

  const mapped_type   call_command_d(key_type key, core::Download* download, const mapped_type& arg)  { return call_command(key, arg, target_type((int)command_base::target_download, download)); }
  const mapped_type   call_command_p(key_type key, torrent::Peer* peer, const mapped_type& arg)       { return call_command(key, arg, target_type((int)command_base::target_peer, peer)); }
  const mapped_type   call_command_t(key_type key, torrent::Tracker* tracker, const mapped_type& arg) { return call_command(key, arg, target_type((int)command_base::target_tracker, tracker)); }
  const mapped_type   call_command_f(key_type key, torrent::File* file, const mapped_type& arg)       { return call_command(key, arg, target_type((int)command_base::target_file, file)); }

private:
  CommandMap(const CommandMap&);
  void operator = (const CommandMap&);
};

inline target_type make_target()                                  { return target_type((int)command_base::target_generic, NULL); }
inline target_type make_target(int type, void* target)            { return target_type(type, target); }
inline target_type make_target(int type, void* target1, void* target2) { return target_type(type, target1, target2); }

template <typename T>
inline target_type make_target(T target) {
  return target_type((int)target_type_id<T>::value, target);
}

template <typename T>
inline target_type make_target_pair(T target1, T target2) {
  return target_type((int)target_type_id<T, T>::value, target1, target2);
}

// TODO: Helper-functions that really should be in the
// torrent/object.h header.

inline torrent::Object
create_object_list(const torrent::Object& o1, const torrent::Object& o2) {
  torrent::Object tmp = torrent::Object::create_list();
  tmp.as_list().push_back(o1);
  tmp.as_list().push_back(o2);
  return tmp;
}

inline torrent::Object
create_object_list(const torrent::Object& o1, const torrent::Object& o2, const torrent::Object& o3) {
  torrent::Object tmp = torrent::Object::create_list();
  tmp.as_list().push_back(o1);
  tmp.as_list().push_back(o2);
  tmp.as_list().push_back(o3);
  return tmp;
}

inline const CommandMap::mapped_type
CommandMap::call(key_type key, const mapped_type& args) {
  return call_command(key, args, make_target());
}

}

#endif
