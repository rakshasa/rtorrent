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
ExecFile   execFile;

struct command_map_is_space : std::unary_function<char, bool> {
  bool operator () (char c) const {
    return c == ' ' || c == '\t';
  }
};

struct command_map_is_newline : std::unary_function<char, bool> {
  bool operator () (char c) const {
    return c == '\n' || c == '\0' || c == ';';
  }
};

// Only escape eol on odd number of escape characters. We know that
// there can't be any characters in between, so this should work for
// all cases.
int
parse_count_escaped(const char* first, const char* last) {
  int escaped = 0;

  while (last != first && *--last == '\\')
    escaped++;

  return escaped;
}

// Replace any strings starting with '$' with the result of the
// result of the command.
//
// Find a better name.
void
parse_command_execute(CommandMap::target_type target, torrent::Object* object) {
  if (object->is_list()) {
    for (torrent::Object::list_type::iterator itr = object->as_list().begin(), last = object->as_list().end(); itr != last; itr++)
      parse_command_execute(target, &*itr);

  } else if (*object->as_string().c_str() == '$') {
    const std::string& str = object->as_string();

    *object = parse_command(target, str.c_str() + 1, str.c_str() + str.size()).first;
  }
}

// Set 'download' to NULL to call the generic functions, thus reusing
// the code below for both cases.
parse_command_type
parse_command(CommandMap::target_type target, const char* first, const char* last) {
  first = std::find_if(first, last, std::not1(command_map_is_space()));

  if (first == last || *first == '#')
    return std::make_pair(torrent::Object(), first);
  
  std::string key;
  first = parse_command_name(first, last, &key);
  first = std::find_if(first, last, std::not1(command_map_is_space()));
  
  if (first == last || *first != '=')
    throw torrent::input_error("Could not find '='.");

  torrent::Object args;
  first = parse_whole_list(first + 1, last, &args);

  // Find the last character that is part of this command, skipping
  // the whitespace at the end. This ensures us that the caller
  // doesn't need to do this nor check for junk at the end.
  first = std::find_if(first, last, std::not1(command_map_is_space()));
  
  if (first != last) {
    if (*first != '\n' && *first != ';' && *first != '\0')
      throw torrent::input_error("Junk at end of input.");

    first++;
  }

  // Replace any strings starting with '$' with the result of the
  // following command.
  parse_command_execute(target, &args);

  return std::make_pair(commands.call_command(key.c_str(), args, target), first);
}

void
parse_command_multiple(CommandMap::target_type target, const char* first, const char* last) {
  while (first != last) {
    // Should we check the return value? Probably not necessary as
    // parse_args throws on unquoted multi-word input.
    parse_command_type result = parse_command(target, first, last);

    first = result.second;
  }
}

bool
parse_command_file(const std::string& path) {
  std::fstream file(rak::path_expand(path).c_str(), std::ios::in);

  if (!file.is_open())
    return false;

  unsigned int lineNumber = 0;
  char buffer[4096];

  try {
    unsigned int getCount = 0;

    while (file.getline(buffer + getCount, 4096 - getCount).good()) {
      if (file.gcount() == 0)
        throw torrent::internal_error("parse_command_file(...) file.gcount() == 0.");

      int escaped = parse_count_escaped(buffer + getCount, buffer + getCount + file.gcount() - 1);

      lineNumber++;
      getCount += file.gcount() - 1;

      if (getCount == 4096 - 1)
        throw torrent::input_error("Exceeded max line lenght.");
      
      if (escaped & 0x1) {
        // Remove the escape characters and continue reading.
        getCount -= escaped;
        continue;
      }

      // Would be nice to make this zero-copy.
      parse_command(make_target(), buffer, buffer + getCount);
      getCount = 0;
    }

  } catch (torrent::input_error& e) {
    snprintf(buffer, 2048, "Error in option file: %s:%u: %s", path.c_str(), lineNumber, e.what());

    throw torrent::input_error(buffer);
  }

  return true;
}

// Use a static length buffer for dest.
const char*
parse_command_name(const char* first, const char* last, std::string* dest) {
  if (first == last || !std::isalpha(*first))
    throw torrent::input_error("Invalid start of name.");

  for ( ; first != last && (std::isalnum(*first) || *first == '_' || *first == '.'); ++first)
    dest->push_back(*first);

  return first;
}

}
