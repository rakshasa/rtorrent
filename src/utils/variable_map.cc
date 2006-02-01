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

#include <cctype>
#include <algorithm>
#include <rak/functional.h>
#include <torrent/exceptions.h>
#include <torrent/bencode.h>

#include "variable.h"
#include "variable_map.h"

namespace utils {

VariableMap::~VariableMap() {
  std::for_each(base_type::begin(), base_type::end(), rak::on(rak::mem_ptr_ref(&value_type::second), rak::call_delete<Variable>()));
}

void
VariableMap::insert(const std::string& key, Variable* v) {
  iterator itr = base_type::find(key);

  if (itr != base_type::end())
    throw torrent::internal_error("VariableMap::insert(...) tried to insert an already existing key.");

  base_type::insert(itr, value_type(key, v));
}

const VariableMap::mapped_type&
VariableMap::get(const std::string& key) {
  iterator itr = base_type::find(key);

  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + key + "\" does not exist.");

  return itr->second->get();
}

void
VariableMap::set(const std::string& key, const mapped_type& arg) {
  iterator itr = base_type::find(key);

  // Later, allow the user to create new variables. Have a slot to
  // register that thing.
  if (itr == base_type::end())
    throw torrent::input_error("Variable \"" + key + "\" does not exist.");

  itr->second->set(arg);
}

std::string::const_iterator
parse_name(std::string::const_iterator first, std::string::const_iterator last, std::string* dest) {
  if (first == last || !std::isalpha(*first))
    throw torrent::input_error("Invalid start of name.");

  for ( ; first != last && (std::isalnum(*first) || *first == '_'); ++first)
    dest->push_back(*first);

  return first;
}

std::string::const_iterator
parse_unknown(std::string::const_iterator first, std::string::const_iterator last, VariableMap::mapped_type* dest) {
  if (*first == '"') {
    std::string::const_iterator next = std::find_if(++first, last, std::bind2nd(std::equal_to<char>(), '"'));
    
    if (first == last ||
	first == next ||
	next == last)
      throw torrent::input_error("Could not find closing '\"'.");

    *dest = std::string(first, next);
    return ++next;

  } else {
    // Add rak::or and check for ','.
    std::string::const_iterator next = std::find_if(first, last, std::ptr_fun(&std::isspace));

    *dest = std::string(first, next);
    return next;
  }
}

std::string::const_iterator
parse_args(std::string::const_iterator first, std::string::const_iterator last, VariableMap::mapped_type::List* dest) {
  first = std::find_if(first, last, std::not1(std::ptr_fun(&std::isspace)));

  while (first != last) {
    dest->push_back(VariableMap::mapped_type());

    first = parse_unknown(first, last, &dest->back());
    first = std::find_if(first, last, std::not1(std::ptr_fun(&std::isspace)));

    if (first != last && *first != ',')
      throw torrent::input_error("A string with blanks must be quoted.");
  }

  return first;
}

void
VariableMap::process_command(const std::string& command) {
  std::string::const_iterator pos = command.begin();
  pos = std::find_if(pos, command.end(), std::not1(std::ptr_fun(&std::isspace)));

  if (pos == command.end() || *pos == '#')
    return;

  // Replace with parse_unknown?
  std::string key;
  pos = parse_name(pos, command.end(), &key);
  pos = std::find_if(pos, command.end(), std::not1(std::ptr_fun(&std::isspace)));
  
  if (pos == command.end() || *pos != '=')
    throw torrent::input_error("Could not find '='.");

  mapped_type args(mapped_type::TYPE_LIST);
  parse_args(pos + 1, command.end(), &args.as_list());

  if (args.as_list().empty())
    set(key, mapped_type());

  else if (++args.as_list().begin() == args.as_list().end())
    set(key, *args.as_list().begin());

  else
    set(key, args);
}

}
