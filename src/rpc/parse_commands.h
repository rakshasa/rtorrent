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

#ifndef RTORRENT_RPC_PARSE_COMMANDS_H
#define RTORRENT_RPC_PARSE_COMMANDS_H

#include <string>

#include "command_map.h"
#include "exec_file.h"
#include "xmlrpc.h"

namespace core {
  class Download;
}

namespace rpc {

// Move to another file?
extern CommandMap commands;
extern XmlRpc     xmlrpc;
extern ExecFile   execFile;

// The generic parse command function, used by the rest. At some point
// the 'download' parameter should be replaced by a more generic one.
std::pair<torrent::Object, const char*> parse_command(core::Download* download, const char* first, const char* last);

void                   parse_command_single(const char* first);
inline torrent::Object parse_command_single(const char* first, const char* last)                             { return parse_command(NULL, first, last).first; }
inline torrent::Object parse_command_d_single(core::Download* download, const char* first, const char* last) { return parse_command(download, first, last).first; }

void                   parse_command_multiple(core::Download* download, const char* first, const char* last);

void                   parse_command_d_multiple(core::Download* download, const char* first);
inline void            parse_command_d_multiple_std(core::Download* download, const std::string& cmd) { parse_command_d_multiple(download, cmd.c_str()); }
inline void            parse_command_multiple(const char* first)                                      { parse_command_d_multiple(NULL, first); }

bool                   parse_command_file(const std::string& path);
const char*            parse_command_name(const char* first, const char* last, std::string* dest);

inline void
parse_command_single_std(const std::string& cmd) {
  parse_command_single(cmd.c_str(), cmd.c_str() + cmd.size());
}

inline void
parse_command_d_single_std(core::Download* download, const std::string& cmd) {
  parse_command_d_single(download, cmd.c_str(), cmd.c_str() + cmd.size());
}

inline torrent::Object call_command(const char* key, const torrent::Object& obj) { return commands.call_command(key, obj); }
inline torrent::Object call_command_void(const char* key)   { return commands.call_command(key, torrent::Object()); }
inline std::string     call_command_string(const char* key) { return commands.call_command(key, torrent::Object()).as_string(); }
inline int64_t         call_command_value(const char* key)  { return commands.call_command(key, torrent::Object()).as_value(); }

inline void            call_command_set_string(const char* key, const std::string& arg)            { commands.call_command(key, torrent::Object(arg)); }
inline void            call_command_set_std_string(const std::string& key, const std::string& arg) { commands.call_command(key.c_str(), torrent::Object(arg)); }

inline torrent::Object call_command_d(const char* key, core::Download* download, const torrent::Object& obj) { return commands.call_command_d(key, download, obj); }
inline torrent::Object call_command_d_void(const char* key, core::Download* download)   { return commands.call_command_d(key, download, torrent::Object()); }
inline std::string     call_command_d_string(const char* key, core::Download* download) { return commands.call_command_d(key, download, torrent::Object()).as_string(); }
inline int64_t         call_command_d_value(const char* key, core::Download* download)  { return commands.call_command_d(key, download, torrent::Object()).as_value(); }

inline void            call_command_d_v_void(const char* key, core::Download* download) { commands.call_command_d(key, download, torrent::Object()); }

inline void            call_command_d_set_value(const char* key, core::Download* download, int64_t arg)                        { commands.call_command_d(key, download, torrent::Object(arg)); }
inline void            call_command_d_set_string(const char* key, core::Download* download, const std::string& arg)            { commands.call_command_d(key, download, torrent::Object(arg)); }
inline void            call_command_d_set_std_string(const std::string& key, core::Download* download, const std::string& arg) { commands.call_command_d(key.c_str(), download, torrent::Object(arg)); }

inline torrent::Object call_command_f(const char* key, torrent::File* file, const torrent::Object& obj) { return commands.call_command_f(key, file, obj); }

inline torrent::Object
call_command_d_range(const char* key, core::Download* download, torrent::Object::list_type::const_iterator first, torrent::Object::list_type::const_iterator last) {
  torrent::Object rawArgs(torrent::Object::TYPE_LIST);
  torrent::Object::list_type& args = rawArgs.as_list();
  
  while (first != last)
    args.push_back(*first++);

  return commands.call_command_d(key, download, rawArgs);
}

}

#endif
