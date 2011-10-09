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

#ifndef RTORRENT_RPC_IP_TABLE_LISTS_H
#define RTORRENT_RPC_IP_TABLE_LISTS_H

#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <torrent/utils/extents.h>

namespace rpc {

typedef torrent::extents<uint32_t, int, 32, 256, 8> ipv4_table;

struct ip_table_node {
  std::string name;
  ipv4_table  table;

  bool equal_name(const std::string& str) const { return str == name; }
};

class ip_table_list : private std::vector<ip_table_node> {
public:
  typedef std::vector<ip_table_node> base_type;

  using base_type::iterator;
  using base_type::const_iterator;
  using base_type::value_type;

  using base_type::begin;
  using base_type::end;
  
  iterator insert(const std::string& name);
  iterator find(const std::string& name);
};

inline ip_table_list::iterator
ip_table_list::insert(const std::string& name) {
  ip_table_node tmp = { name };

  return base_type::insert(end(), tmp);
}

inline ip_table_list::iterator
ip_table_list::find(const std::string& name) {
  for (iterator itr = begin(), last = end(); itr != last; itr++)
    if (itr->equal_name(name))
      return itr;

  return end();
}

}

#endif
