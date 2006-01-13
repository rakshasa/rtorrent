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

#ifndef RTORRENT_UTILS_VARIABLE_MAP_H
#define RTORRENT_UTILS_VARIABLE_MAP_H

#include <map>
#include <string>
#include <torrent/bencode.h>

namespace utils {

class Variable;

class VariableMap : public std::map<std::string, Variable*> {
public:
  typedef std::map<std::string, Variable*> base_type;

  using base_type::iterator;
  using base_type::value_type;

  VariableMap() {}
  ~VariableMap();

  void                insert(const std::string& key, Variable* v);

  // Consider taking char* start and finish instead of std::string to
  // avoid copying. Or make a view class.
  const torrent::Bencode& get(const std::string& key);

  void                set(const std::string& key, const torrent::Bencode& arg);
  void                set_string(const std::string& key, const std::string& arg) { set(key, torrent::Bencode(arg)); }

  // Temporary.
  void                process_command(const std::string& command);

private:
  VariableMap(const VariableMap&);
  void operator = (const VariableMap&);
};

}

#endif
