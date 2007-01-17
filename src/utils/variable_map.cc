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
#include <cstdio>
#include <cctype>
#include <fstream>
#include <rak/functional.h>
#include <rak/path.h>
#include <torrent/exceptions.h>
#include <torrent/object.h>

#include "variable.h"
#include "variable_map.h"

namespace utils {

VariableMap::~VariableMap() {
  std::for_each(base_type::begin(), base_type::end(), rak::on(rak::mem_ref(&value_type::second), rak::call_delete<Variable>()));
}

void
VariableMap::insert(key_type key, Variable* v) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("VariableMap::insert(...) tried to insert an already existing key.");

  base_type::insert(itr, value_type(key, v));
}

const VariableMap::mapped_type&
VariableMap::get(key_type key) const {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  return itr->second->get();
}

const VariableMap::mapped_type&
VariableMap::get_d(core::Download* download, key_type key) const {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  return itr->second->get_d(download);
}

void
VariableMap::set(key_type key, const mapped_type& arg) {
  iterator itr = base_type::find(key);

  // Later, allow the user to create new variables. Have a slot to
  // register that thing.
  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  itr->second->set(arg);
}

void
VariableMap::set_d(core::Download* download, key_type key, const mapped_type& arg) {
  iterator itr = base_type::find(key);

  // Later, allow the user to create new variables. Have a slot to
  // register that thing.
  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  itr->second->set_d(download, arg);
}

struct variable_map_is_space : std::unary_function<char, bool> {
  bool operator () (char c) const {
    return std::isspace(c);
  }
};

struct variable_map_is_newline : std::unary_function<char, bool> {
  bool operator () (char c) const {
    return c == '\n' || c == '\0';
  }
};

// Use a static length buffer for dest.
const char*
parse_name(const char* first, const char* last, std::string* dest) {
  if (first == last || !std::isalpha(*first))
    throw torrent::input_error("Invalid start of name.");

  for ( ; first != last && (std::isalnum(*first) || *first == '_'); ++first)
    dest->push_back(*first);

  return first;
}

const char*
parse_unknown(const char* first, const char* last, VariableMap::mapped_type* dest) {
  if (*first == '"') {
    const char* next = std::find_if(++first, last, std::bind2nd(std::equal_to<char>(), '"'));
    
    if (first == last || first == next || next == last)
      throw torrent::input_error("Could not find closing '\"'.");

    *dest = std::string(first, next);
    return ++next;

  } else {
    // Add rak::or and check for ','.
    const char* next = std::find_if(first, last, variable_map_is_space());

    *dest = std::string(first, next);
    return next;
  }
}

const char*
parse_args(const char* first, const char* last, VariableMap::mapped_type::list_type* dest) {
  first = std::find_if(first, last, std::not1(variable_map_is_space()));

  while (first != last) {
    dest->push_back(VariableMap::mapped_type());

    first = parse_unknown(first, last, &dest->back());
    first = std::find_if(first, last, std::not1(variable_map_is_space()));

    if (first != last && *first != ',')
      throw torrent::input_error("A string with blanks must be quoted.");
  }

  return first;
}

const char*
VariableMap::process_single(const char* first) {
  // This could be optimized, but dunno if there's any point in doing
  // it.
  return process_single(first, first + std::strlen(first));
}

const char*
VariableMap::process_single(const char* first, const char* last) {
  first = std::find_if(first, last, std::not1(variable_map_is_space()));

  if (first == last || *first == '#')
    return last;
  
  std::string key;
  first = parse_name(first, last, &key);
  first = std::find_if(first, last, std::not1(variable_map_is_space()));
  
  if (first == last || *first != '=')
    throw torrent::input_error("Could not find '='.");

  mapped_type args(mapped_type::TYPE_LIST);
  first = parse_args(first + 1, last, &args.as_list());

  if (args.as_list().empty())
    set(key.c_str(), mapped_type());

  else if (++args.as_list().begin() == args.as_list().end())
    set(key.c_str(), *args.as_list().begin());

  else
    set(key.c_str(), args);

  return std::find_if(first, last, std::not1(variable_map_is_space()));
}

// void
// VariableMap::process_command(const std::string& command) {
//   std::string::const_iterator pos = command.begin();
//   pos = std::find_if(pos, command.end(), std::not1(variable_map_is_space()));

//   if (pos == command.end() || *pos == '#')
//     return;

//   // Replace with parse_unknown?
//   std::string key;
//   pos = parse_name(pos, command.end(), &key);
//   pos = std::find_if(pos, command.end(), std::not1(variable_map_is_space()));
  
//   if (pos == command.end() || *pos != '=')
//     throw torrent::input_error("Could not find '='.");

//   mapped_type args(mapped_type::TYPE_LIST);
//   parse_args(pos + 1, command.end(), &args.as_list());

//   if (args.as_list().empty())
//     set(key.c_str(), mapped_type());

//   else if (++args.as_list().begin() == args.as_list().end())
//     set(key.c_str(), *args.as_list().begin());

//   else
//     set(key.c_str(), args);


// Consider what the command scheduler should do.

void
VariableMap::process_multiple(const char* first) {
  while (first != '\0') {
    const char* last = first;

    while (*last != '\n' && *last != '\0') last++;

    // Should we check the return value? Probably not necessary as
    // parse_args throws on unquoted multi-word input.
    process_single(first, last);

    if (*last == '\0')
      return;

    first = last + 1;
  }
}

bool
VariableMap::process_file(key_type path) {
  std::fstream file(rak::path_expand(path).c_str(), std::ios::in);

  if (!file.is_open())
    return false;

  int lineNumber = 0;
  char buffer[max_size_line];

  try {

    while (file.getline(buffer, max_size_line).good()) {
      lineNumber++;
      // Would be nice to make this zero-copy.
      process_single(buffer, buffer + std::strlen(buffer));
    }

  } catch (torrent::input_error& e) {
    snprintf(buffer, max_size_line, "Error in option file: %s:%i: %s", path, lineNumber, e.what());

    throw torrent::input_error(buffer);
  }

  return true;
}

}
