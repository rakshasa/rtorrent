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
parse_command_execute(target_type target, torrent::Object* object) {
  if (object->is_list()) {
    // For now, until we can flag the lists we want executed and those
    // we can't, disable recursion completely.
    for (torrent::Object::list_iterator itr = object->as_list().begin(), last = object->as_list().end(); itr != last; itr++) {
      if (itr->is_list())
        continue;

      parse_command_execute(target, &*itr);
    }

  } else if (object->is_dict_key()) {
    parse_command_execute(target, &object->as_dict_obj());

    if (object->flags() & torrent::Object::flag_function) {
      *object = rpc::commands.call_command(object->as_dict_key().c_str(), object->as_dict_obj(), target);

    } else {
      uint32_t flags = object->flags() & torrent::Object::mask_function;
      object->unset_flags(torrent::Object::mask_function);
      object->set_flags((flags >> 1) & torrent::Object::mask_function);
    }

  } else if (object->is_string() && *object->as_string().c_str() == '$') {
    const std::string& str = object->as_string();

    *object = parse_command(target, str.c_str() + 1, str.c_str() + str.size()).first;
  }
}

// Use a static length buffer for dest.
inline const char*
parse_command_name(const char* first, const char* last, char* dest_first, char* dest_last) {
  if (first == last || !std::isalpha(*first))
    throw torrent::input_error("Invalid start of command name.");

  last = first + std::min(std::distance(first, last), std::distance(dest_first, dest_last) - 1);

  while (first != last && (std::isalnum(*first) || *first == '_' || *first == '.'))
    *dest_first++ = *first++;

  *dest_first = '\0';
  return first;
}

// Set 'download' to NULL to call the generic functions, thus reusing
// the code below for both cases.
parse_command_type
parse_command(target_type target, const char* first, const char* last) {
  first = std::find_if(first, last, std::not1(command_map_is_space()));

  if (first == last || *first == '#')
    return std::make_pair(torrent::Object(), first);
  
  char key[128];

  first = parse_command_name(first, last, key, key + 128);
  first = std::find_if(first, last, std::not1(command_map_is_space()));
  
  if (first == last || *first != '=')
    throw torrent::input_error("Could not find '=' in command '" + std::string(key) + "'.");

  torrent::Object args;
  first = parse_whole_list(first + 1, last, &args, &parse_is_delim_command);

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

  return std::make_pair(commands.call_command(key, args, target), first);
}

torrent::Object
parse_command_multiple(target_type target, const char* first, const char* last) {
  parse_command_type result;

  while (first != last) {
    // Should we check the return value? Probably not necessary as
    // parse_args throws on unquoted multi-word input.
    result = parse_command(target, first, last);

    first = result.second;
  }

  return result.first;
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

    while (file.good()
           && !file.getline(buffer + getCount, 4096 - getCount).fail()) {
      
      if (file.gcount() == 0)
        throw torrent::internal_error("parse_command_file(...) file.gcount() == 0.");
      int lineLength = file.gcount() - 1;
      // In case we are at the end of the file and the last character is
      // not a line feed, we'll just increase the read character count so 
      // that the last would also be included in option line.
      if (file.eof() && file.get() != '\n')
        lineLength++;
      
      int escaped = parse_count_escaped(buffer + getCount, buffer + getCount + lineLength);

      lineNumber++;
      getCount += lineLength;

      if (getCount == 4096 - 1)
        throw torrent::input_error("Exceeded max line length.");
      
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

torrent::Object
call_object(const torrent::Object& command, target_type target) {
  switch (command.type()) {
  case torrent::Object::TYPE_RAW_STRING:
    return parse_command_multiple(target, command.as_raw_string().begin(), command.as_raw_string().end());
  case torrent::Object::TYPE_STRING:
    return parse_command_multiple(target, command.as_string().c_str(), command.as_string().c_str() + command.as_string().size());

  case torrent::Object::TYPE_LIST:
  {
    torrent::Object result;

    for (torrent::Object::list_const_iterator itr = command.as_list().begin(), last = command.as_list().end(); itr != last; itr++)
      result = call_object(*itr, target);

    return result;
  }
  case torrent::Object::TYPE_MAP:
  {
    for (torrent::Object::map_const_iterator itr = command.as_map().begin(), last = command.as_map().end(); itr != last; itr++)
      call_object(itr->second, target);

    return torrent::Object();
  }
  case torrent::Object::TYPE_DICT_KEY:
  {
    // This can/should be optimized...
    torrent::Object tmp_command = command;

    // Unquote the root function object so 'parse_command_execute'
    // doesn't end up calling it.
    //
    // TODO: Only call this if mask_function is set?
    uint32_t flags = tmp_command.flags() & torrent::Object::mask_function;
    tmp_command.unset_flags(torrent::Object::mask_function);
    tmp_command.set_flags((flags >> 1) & torrent::Object::mask_function);

    parse_command_execute(target, &tmp_command);
    return commands.call_command(tmp_command.as_dict_key().c_str(), tmp_command.as_dict_obj(), target);
  }
  default:
    return torrent::Object();
  }
}

//
//
//

const torrent::Object
command_function_call_object(const torrent::Object& cmd, target_type target, const torrent::Object& args) {
  rpc::command_base::stack_type stack;
  torrent::Object* last_stack;

  if (args.is_list())
    last_stack = rpc::command_base::push_stack(args.as_list(), &stack);
  else if (args.type() != torrent::Object::TYPE_NONE)
    last_stack = rpc::command_base::push_stack(&args, &args + 1, &stack);
  else
    last_stack = rpc::command_base::push_stack(NULL, NULL, &stack);

  try {
    torrent::Object result = call_object(cmd, target);
    rpc::command_base::pop_stack(&stack, last_stack);
    return result;

  } catch (torrent::bencode_error& e) {
    rpc::command_base::pop_stack(&stack, last_stack);
    throw e;
  }
}

}
