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

#ifndef RTORRENT_UTILS_COMMAND_HELPERS_H
#define RTORRENT_UTILS_COMMAND_HELPERS_H

#include "rpc/command.h"
#include "rpc/parse_commands.h"
#include "rpc/object_storage.h"

void initialize_commands();

//
// New std::function based command_base helper functions:
//

#define CMD2_A_FUNCTION(key, function, slot, parm, doc)                 \
  rpc::commands.insert_slot<rpc::command_base_is_type<rpc::function>::type>(key, slot, &rpc::function, \
                            rpc::CommandMap::flag_dont_delete | rpc::CommandMap::flag_public_xmlrpc, NULL, NULL);

#define CMD2_A_FUNCTION_PRIVATE(key, function, slot, parm, doc)         \
  rpc::commands.insert_slot<rpc::command_base_is_type<rpc::function>::type>(key, slot, &rpc::function,   \
                            rpc::CommandMap::flag_dont_delete, NULL, NULL);

#define CMD2_ANY(key, slot)          CMD2_A_FUNCTION(key, command_base_call<rpc::target_type>, slot, "i:", "")

#define CMD2_ANY_P(key, slot)        CMD2_A_FUNCTION_PRIVATE(key, command_base_call<rpc::target_type>, slot, "i:", "")
#define CMD2_ANY_VOID(key, slot)     CMD2_A_FUNCTION(key, command_base_call<rpc::target_type>, object_convert_void(slot), "i:", "")
#define CMD2_ANY_V(key, slot)        CMD2_A_FUNCTION(key, command_base_call_list<rpc::target_type>, object_convert_void(slot), "i:", "")
#define CMD2_ANY_L(key, slot)        CMD2_A_FUNCTION(key, command_base_call_list<rpc::target_type>, slot, "A:", "")

#define CMD2_ANY_VALUE(key, slot)    CMD2_A_FUNCTION(key, command_base_call_value<rpc::target_type>, slot, "i:i", "")
#define CMD2_ANY_VALUE_V(key, slot)  CMD2_A_FUNCTION(key, command_base_call_value<rpc::target_type>, object_convert_void(slot), "i:i", "")
#define CMD2_ANY_VALUE_KB(key, slot) CMD2_A_FUNCTION(key, command_base_call_value_kb<rpc::target_type>, object_convert_void(slot), "i:i", "")

#define CMD2_ANY_STRING(key, slot)   CMD2_A_FUNCTION(key, command_base_call_string<rpc::target_type>, slot, "i:s", "")
#define CMD2_ANY_STRING_V(key, slot) CMD2_A_FUNCTION(key, command_base_call_string<rpc::target_type>, object_convert_void(slot), "i:s", "")

#define CMD2_ANY_LIST(key, slot)     CMD2_A_FUNCTION(key, command_base_call_list<rpc::target_type>, slot, "i:", "")

#define CMD2_DL(key, slot)           CMD2_A_FUNCTION(key, command_base_call<core::Download*>, slot, "i:", "")
#define CMD2_DL_V(key, slot)         CMD2_A_FUNCTION(key, command_base_call<core::Download*>, object_convert_void(slot), "i:", "")
#define CMD2_DL_VALUE(key, slot)     CMD2_A_FUNCTION(key, command_base_call_value<core::Download*>, slot, "i:", "")
#define CMD2_DL_VALUE_V(key, slot)   CMD2_A_FUNCTION(key, command_base_call_value<core::Download*>, object_convert_void(slot), "i:", "")
#define CMD2_DL_STRING(key, slot)    CMD2_A_FUNCTION(key, command_base_call_string<core::Download*>, slot, "i:", "")
#define CMD2_DL_STRING_V(key, slot)  CMD2_A_FUNCTION(key, command_base_call_string<core::Download*>, object_convert_void(slot), "i:", "")
#define CMD2_DL_LIST(key, slot)      CMD2_A_FUNCTION(key, command_base_call_list<core::Download*>, slot, "i:", "")

#define CMD2_DL_VALUE_P(key, slot)   CMD2_A_FUNCTION_PRIVATE(key, command_base_call_value<core::Download*>, slot, "i:", "")
#define CMD2_DL_STRING_P(key, slot)  CMD2_A_FUNCTION_PRIVATE(key, command_base_call_string<core::Download*>, slot, "i:", "")

#define CMD2_FILE(key, slot)         CMD2_A_FUNCTION(key, command_base_call<torrent::File*>, slot, "i:", "")
#define CMD2_FILE_V(key, slot)       CMD2_A_FUNCTION(key, command_base_call<torrent::File*>, object_convert_void(slot), "i:", "")
#define CMD2_FILE_VALUE_V(key, slot) CMD2_A_FUNCTION(key, command_base_call_value<torrent::File*>, object_convert_void(slot), "i:i", "")

#define CMD2_FILEITR(key, slot)         CMD2_A_FUNCTION(key, command_base_call<torrent::FileListIterator*>, slot, "i:", "")

#define CMD2_PEER(key, slot)            CMD2_A_FUNCTION(key, command_base_call<torrent::Peer*>, slot, "i:", "")
#define CMD2_PEER_V(key, slot)          CMD2_A_FUNCTION(key, command_base_call<torrent::Peer*>, object_convert_void(slot), "i:", "")
#define CMD2_PEER_VALUE_V(key, slot)    CMD2_A_FUNCTION(key, command_base_call_value<torrent::Peer*>, object_convert_void(slot), "i:i", "")

#define CMD2_TRACKER(key, slot)         CMD2_A_FUNCTION(key, command_base_call<torrent::Tracker*>, slot, "i:", "")
#define CMD2_TRACKER_V(key, slot)       CMD2_A_FUNCTION(key, command_base_call<torrent::Tracker*>, object_convert_void(slot), "i:", "")
#define CMD2_TRACKER_VALUE_V(key, slot) CMD2_A_FUNCTION(key, command_base_call_value<torrent::Tracker*>, object_convert_void(slot), "i:i", "")

#define CMD2_VAR_BOOL(key, value)                                       \
  control->object_storage()->insert_c_str(key, int64_t(value), rpc::object_storage::flag_bool_type); \
  CMD2_ANY(key, std::bind(&rpc::object_storage::get, control->object_storage(), \
                               torrent::raw_string::from_c_str(key)));  \
  CMD2_ANY_VALUE(key ".set", std::bind(&rpc::object_storage::set_bool, control->object_storage(), \
                                            torrent::raw_string::from_c_str(key), std::placeholders::_2));

#define CMD2_VAR_VALUE(key, value)                                      \
  control->object_storage()->insert_c_str(key, int64_t(value), rpc::object_storage::flag_value_type); \
  CMD2_ANY(key, std::bind(&rpc::object_storage::get, control->object_storage(), \
                               torrent::raw_string::from_c_str(key)));  \
  CMD2_ANY_VALUE(key ".set", std::bind(&rpc::object_storage::set_value, control->object_storage(), \
                                            torrent::raw_string::from_c_str(key), std::placeholders::_2));

#define CMD2_VAR_STRING(key, value)                                     \
  control->object_storage()->insert_c_str(key, value, rpc::object_storage::flag_string_type); \
  CMD2_ANY(key, std::bind(&rpc::object_storage::get, control->object_storage(), \
                               torrent::raw_string::from_c_str(key)));  \
  CMD2_ANY_STRING(key ".set", std::bind(&rpc::object_storage::set_string, control->object_storage(), \
                                             torrent::raw_string::from_c_str(key), std::placeholders::_2));


#define CMD2_VAR_C_STRING(key, value)                                   \
  control->object_storage()->insert_c_str(key, value, rpc::object_storage::flag_string_type); \
  CMD2_ANY(key, std::bind(&rpc::object_storage::get, control->object_storage(), \
                               torrent::raw_string::from_c_str(key)));

#define CMD2_VAR_LIST(key)                                              \
  control->object_storage()->insert_c_str(key, torrent::Object::create_list(), rpc::object_storage::flag_list_type); \
  CMD2_ANY(key, std::bind(&rpc::object_storage::get, control->object_storage(), \
                               torrent::raw_string::from_c_str(key)));  \
  CMD2_ANY_LIST(key ".set", std::bind(&rpc::object_storage::set_list, control->object_storage(), \
                                           torrent::raw_string::from_c_str(key), std::placeholders::_2)); \
  CMD2_ANY_VOID(key ".push_back", std::bind(&rpc::object_storage::list_push_back, control->object_storage(), \
                                                 torrent::raw_string::from_c_str(key), std::placeholders::_2));

#define CMD2_FUNC_SINGLE(key, cmds)                                     \
  CMD2_ANY(key, std::bind(&rpc::command_function_call_object, torrent::Object(torrent::raw_string::from_c_str(cmds)), \
                               std::placeholders::_1, std::placeholders::_2));

#define CMD2_REDIRECT(from_key, to_key) \
  rpc::commands.create_redirect(from_key, to_key, rpc::CommandMap::flag_public_xmlrpc | rpc::CommandMap::flag_dont_delete);
#define CMD2_REDIRECT_GENERIC(from_key, to_key) \
  rpc::commands.create_redirect(from_key, to_key, rpc::CommandMap::flag_public_xmlrpc | rpc::CommandMap::flag_no_target | rpc::CommandMap::flag_dont_delete);
#define CMD2_REDIRECT_GENERIC_NO_EXPORT(from_key, to_key) \
  rpc::commands.create_redirect(from_key, to_key, rpc::CommandMap::flag_no_target | rpc::CommandMap::flag_dont_delete);
#define CMD2_REDIRECT_FILE(from_key, to_key) \
  rpc::commands.create_redirect(from_key, to_key, rpc::CommandMap::flag_public_xmlrpc | rpc::CommandMap::flag_file_target | rpc::CommandMap::flag_dont_delete);
#define CMD2_REDIRECT_TRACKER(from_key, to_key) \
  rpc::commands.create_redirect(from_key, to_key, rpc::CommandMap::flag_public_xmlrpc | rpc::CommandMap::flag_tracker_target | rpc::CommandMap::flag_dont_delete);

#define CMD2_REDIRECT_GENERIC_STR(from_key, to_key)                     \
  rpc::commands.create_redirect(create_new_key(from_key), create_new_key(to_key), \
                                rpc::CommandMap::flag_public_xmlrpc | rpc::CommandMap::flag_no_target | rpc::CommandMap::flag_delete_key);

#define CMD2_REDIRECT_GENERIC_STR_NO_EXPORT(from_key, to_key)                     \
  rpc::commands.create_redirect(create_new_key(from_key), create_new_key(to_key), \
                                rpc::CommandMap::flag_no_target | rpc::CommandMap::flag_delete_key);

//
// Conversion of return types:
//

template <typename Functor, typename Result>
struct object_convert_type;

template <typename Functor>
struct object_convert_type<Functor, void> {

  template <typename Signature> struct result {
    typedef torrent::Object type;
  };

  object_convert_type(Functor s) : m_slot(s) {}

  torrent::Object operator () () { m_slot(); return torrent::Object(); }
  template <typename Arg1>
  torrent::Object operator () (Arg1& arg1) { m_slot(arg1); return torrent::Object(); }
  template <typename Arg1, typename Arg2>
  torrent::Object operator () (const Arg1& arg1) { m_slot(arg1); return torrent::Object(); }
  template <typename Arg1, typename Arg2>
  torrent::Object operator () (Arg1& arg1, Arg2& arg2) { m_slot(arg1, arg2); return torrent::Object(); }
  template <typename Arg1, typename Arg2>
  torrent::Object operator () (const Arg1& arg1, const Arg2& arg2) { m_slot(arg1, arg2); return torrent::Object(); }

  Functor m_slot;
};

template <typename T>
object_convert_type<T, void>
object_convert_void(T f) { return f; }

//
// Key creation:
//

template <int postfix_size>
inline const char*
create_new_key(const std::string& key, const char postfix[postfix_size]) {
  char *buffer = new char[key.size() + std::max(postfix_size, 1)];
  std::memcpy(buffer, key.c_str(), key.size() + 1);
  std::memcpy(buffer + key.size(), postfix, postfix_size);
  return buffer;
}

inline const char*
create_new_key(const std::string& key) {
  char *buffer = new char[key.size() + 1];
  std::memcpy(buffer, key.c_str(), key.size() + 1);
  return buffer;
}

#endif
