// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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

#ifndef RTORRENT_DISPLAY_CLIENT_INFO_H
#define RTORRENT_DISPLAY_CLIENT_INFO_H

#include <string>
#include <vector>
#include <inttypes.h>

namespace display {

class ClientInfo {
public:
  // Largest key size + 1.
  static const uint32_t key_size = 3;

  typedef uint32_t                               size_type;
  typedef std::pair<char[key_size], const char*> value_type;
  typedef std::vector<value_type>                container_type;
  typedef container_type::iterator               iterator;
  typedef container_type::const_iterator         const_iterator;

  typedef enum {
    TYPE_AZUREUS,
    TYPE_COMPACT,
    TYPE_MAINLINE,
    TYPE_MAXSIZE
  } Type;

  ClientInfo();

  void                insert(Type t, const char* key, const char* name);

  char*               print(char* first, char* last, const char* id) const;

  // Fix this...
  std::string         str(const char* id) const;
  std::string         str_str(const std::string& id) const { return str(id.c_str()); }

  size_type           sizeof_key(Type t) const {
    switch (t) {
    case TYPE_AZUREUS:  return 2;
    case TYPE_COMPACT:  return 1;
    case TYPE_MAINLINE: return 1;
    default:
    case TYPE_MAXSIZE:  return 0;
    }
  }

private:
  container_type      m_containers[TYPE_MAXSIZE];
};

}

#endif
