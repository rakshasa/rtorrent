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

#include "parse.h"
#include "variable.h"
#include "variable_map.h"

namespace utils {

struct variable_map_get_ptr : std::unary_function<VariableMap::value_type&, Variable*> {
  Variable* operator () (VariableMap::value_type& value) { return value.second.m_variable; }
};

VariableMap::~VariableMap() {
  for (iterator itr = base_type::begin(), last = base_type::end(); itr != last; itr++)
    if (!(itr->second.m_flags & flag_dont_delete))
      delete itr->second.m_variable;
}

void
VariableMap::insert(key_type key, Variable* variable, generic_slot genericSlot, int flags) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("VariableMap::insert(...) tried to insert an already existing key.");

  base_type::insert(itr, value_type(key, variable_map_data_type(variable, genericSlot, NULL, flags)));
}

const VariableMap::mapped_type&
VariableMap::get(key_type key) const {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_genericSlot != NULL)
    return itr->second.m_genericSlot(itr->second.m_variable, torrent::Object());

  return itr->second.m_variable->get();
}

const VariableMap::mapped_type&
VariableMap::get_d(core::Download* download, key_type key) const {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_downloadSlot != NULL)
    return itr->second.m_downloadSlot(itr->second.m_variable, download, torrent::Object());

  if (itr->second.m_genericSlot != NULL)
    return itr->second.m_genericSlot(itr->second.m_variable, torrent::Object());

  return itr->second.m_variable->get_d(download);
}

void
VariableMap::set(key_type key, const mapped_type& arg) {
  iterator itr = base_type::find(key);

  // Later, allow the user to create new variables. Have a slot to
  // register that thing.
  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_genericSlot != NULL) {
    itr->second.m_genericSlot(itr->second.m_variable, arg);
    return;
  }

  itr->second.m_variable->set(arg);
}

void
VariableMap::set_d(core::Download* download, key_type key, const mapped_type& arg) {
  iterator itr = base_type::find(key);

  // Later, allow the user to create new variables. Have a slot to
  // register that thing.
  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_downloadSlot != NULL) {
    itr->second.m_downloadSlot(itr->second.m_variable, download, arg);
    return;
  }

  if (itr->second.m_genericSlot != NULL) {
    itr->second.m_genericSlot(itr->second.m_variable, arg);
    return;
  }

  itr->second.m_variable->set_d(download, arg);
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

  mapped_type args;
  first = parse_whole_list(first + 1, last, &args);

  set(key.c_str(), args);

  return first;
}

const char*
VariableMap::process_d_single(core::Download* download, const char* first, const char* last) {
  first = std::find_if(first, last, std::not1(variable_map_is_space()));

  if (first == last || *first == '#')
    return last;
  
  std::string key;
  first = parse_name(first, last, &key);
  first = std::find_if(first, last, std::not1(variable_map_is_space()));
  
  if (first == last || *first != '=')
    throw torrent::input_error("Could not find '='.");

  mapped_type args;
  first = parse_whole_list(first + 1, last, &args);

  set_d(download, key.c_str(), args);

  return first;
}

// Consider what the command scheduler should do.

void
VariableMap::process_multiple(const char* first) {
  try {
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

  } catch (torrent::input_error& e) {
    throw torrent::input_error(std::string("Error parsing multi-line option: ") + e.what());
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

const VariableMap::mapped_type&
VariableMap::call_command(key_type key, const mapped_type& arg) {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_genericSlot == NULL)
    throw torrent::input_error("Variable does not have a generic slot.");

  return itr->second.m_genericSlot(itr->second.m_variable, arg);
}

const VariableMap::mapped_type&
VariableMap::call_command_get(key_type key, const mapped_type& arg) {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_genericSlot != NULL)
    return itr->second.m_genericSlot(itr->second.m_variable, arg);

  return get(key);
}

const VariableMap::mapped_type&
VariableMap::call_command_set(key_type key, const mapped_type& arg) {
  const_iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + std::string(key) + "\" does not exist.");

  if (itr->second.m_genericSlot != NULL)
    return itr->second.m_genericSlot(itr->second.m_variable, arg);

  set(key, arg);

  return Variable::m_emptyObject;
}

}
