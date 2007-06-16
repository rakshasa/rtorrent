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

namespace core {
  class Download;
}

namespace utils {

class CommandMap;

const char* parse_command_name(const char* first, const char* last, std::string* dest);

const char* parse_command_single(CommandMap* varMap, const char* first);
const char* parse_command_single(CommandMap* varMap, const char* first, const char* last);

const char* parse_command_d_single(CommandMap* varMap, core::Download* download, const char* first, const char* last);

void        parse_command_multiple(CommandMap* varMap, const char* first);
bool        parse_command_file(CommandMap* varMap, const std::string& path);

inline void
parse_command_single_std(CommandMap* varMap, const std::string& cmd) {
  parse_command_single(varMap, cmd.c_str(), cmd.c_str() + cmd.size());
}

inline void
parse_command_d_single_std(CommandMap* varMap, core::Download* download, const std::string& cmd) {
  parse_command_d_single(varMap, download, cmd.c_str(), cmd.c_str() + cmd.size());
}

}

#endif
