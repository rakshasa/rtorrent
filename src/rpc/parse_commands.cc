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

#include "config.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <rak/functional.h>
#include <rak/path.h>
#include <torrent/exceptions.h>

#include "parse.h"
#include "parse_commands.h"

namespace rpc {

CommandMap commands;
XmlRpc     xmlrpc;

struct command_map_is_space : std::unary_function<char, bool> {
  bool operator () (char c) const {
    return std::isspace(c);
  }
};

struct command_map_is_newline : std::unary_function<char, bool> {
  bool operator () (char c) const {
    return c == '\n' || c == '\0';
  }
};

// Use a static length buffer for dest.
const char*
parse_command_name(const char* first, const char* last, std::string* dest) {
  if (first == last || !std::isalpha(*first))
    throw torrent::input_error("Invalid start of name.");

  for ( ; first != last && (std::isalnum(*first) || *first == '_'); ++first)
    dest->push_back(*first);

  return first;
}

void
parse_command_single(const char* first) {
  parse_command_single(first, first + std::strlen(first));
}

torrent::Object
parse_command_d_single(core::Download* download, const char* first, const char* last) {
  first = std::find_if(first, last, std::not1(command_map_is_space()));

  if (first == last || *first == '#')
    return torrent::Object();
  
  std::string key;
  first = parse_command_name(first, last, &key);
  first = std::find_if(first, last, std::not1(command_map_is_space()));
  
  if (first == last || *first != '=')
    throw torrent::input_error("Could not find '='.");

  torrent::Object args;
  parse_whole_list(first + 1, last, &args);

  if (args.is_list()) {
    for (torrent::Object::list_type::iterator itr = args.as_list().begin(), last = args.as_list().begin(); itr != last; itr++) {
      if (!itr->is_string())
        continue;

      const std::string& str = itr->as_string();

      if (*str.c_str() == '$')
        *itr = parse_command_d_single(download, str.c_str() + 1, str.c_str() + str.size());
    }

  } else if (*args.as_string().c_str() == '$') {
    const std::string& str = args.as_string();

    args = parse_command_d_single(download, str.c_str() + 1, str.c_str() + str.size());
  }

  return commands.call_command_d(key.c_str(), download, args);
}

void
parse_command_multiple(const char* first) {
  try {
    while (first != '\0') {
      const char* last = first;

      while (*last != '\n' && *last != '\0') last++;

      // Should we check the return value? Probably not necessary as
      // parse_args throws on unquoted multi-word input.
      parse_command_single(first, last);

      if (*last == '\0')
        return;

      first = last + 1;
    }

  } catch (torrent::input_error& e) {
    throw torrent::input_error(std::string("Error parsing multi-line option: ") + e.what());
  }
}

bool
parse_command_file(const std::string& path) {
  std::fstream file(rak::path_expand(path).c_str(), std::ios::in);

  if (!file.is_open())
    return false;

  int lineNumber = 0;
  char buffer[2048];

  try {

    while (file.getline(buffer, 2048).good()) {
      lineNumber++;
      // Would be nice to make this zero-copy.
      parse_command_single(buffer, buffer + std::strlen(buffer));
    }

  } catch (torrent::input_error& e) {
    snprintf(buffer, 2048, "Error in option file: %s:%i: %s", path.c_str(), lineNumber, e.what());

    throw torrent::input_error(buffer);
  }

  return true;
}

}
