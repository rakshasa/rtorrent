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

#include "config.h"

#include <cctype>
#include <cstring>
#include <rak/string_manip.h>
#include <torrent/exceptions.h>

#include "client_info.h"
#include "utils.h"

namespace display {

struct client_info_equal {
  client_info_equal(const char* key, ClientInfo::size_type size) : m_key(key), m_size(size) {}

  bool operator () (const ClientInfo::value_type& v) const {
    for (ClientInfo::size_type i = 0; i < m_size; ++i)
      if (v.first[i] != m_key[i])
        return false;

    return true;
  }

  const char*           m_key;
  ClientInfo::size_type m_size;
};

ClientInfo::ClientInfo() {
  m_containers[TYPE_AZUREUS].reserve(30);

  insert(TYPE_AZUREUS, "AZ", "Azureus");
  insert(TYPE_AZUREUS, "BB", "BitBuddy");
  insert(TYPE_AZUREUS, "BC", "BitComet");
  insert(TYPE_AZUREUS, "UT", "uTorrent");
  insert(TYPE_AZUREUS, "lt", "libTorrent");
  insert(TYPE_AZUREUS, "CT", "CTorrent");
  insert(TYPE_AZUREUS, "MT", "MoonlightTorrent");
  insert(TYPE_AZUREUS, "LT", "libtorrent");
  insert(TYPE_AZUREUS, "KT", "KTorrent");
  insert(TYPE_AZUREUS, "BX", "Bittorrent X");
  insert(TYPE_AZUREUS, "TS", "Torrentstorm");
  insert(TYPE_AZUREUS, "TN", "TorrentDotNET");
  insert(TYPE_AZUREUS, "TR", "Transmission");
  insert(TYPE_AZUREUS, "SS", "SwarmScope");
  insert(TYPE_AZUREUS, "XT", "XanTorrent");
  insert(TYPE_AZUREUS, "BS", "BTSlave");
  insert(TYPE_AZUREUS, "ZT", "ZipTorrent");
  insert(TYPE_AZUREUS, "AR", "Arctic");
  insert(TYPE_AZUREUS, "SB", "Swiftbit");
  insert(TYPE_AZUREUS, "MP", "MooPolice");
  insert(TYPE_AZUREUS, "QT", "Qt 4 Torrent example");
  insert(TYPE_AZUREUS, "SZ", "Shareaza");
  insert(TYPE_AZUREUS, "RT", "Retriever");
  insert(TYPE_AZUREUS, "CD", "Enhanced CTorrent");

  m_containers[TYPE_COMPACT].reserve(10);

  insert(TYPE_COMPACT, "A", "ABC");
  insert(TYPE_COMPACT, "T", "BitTornado");
  insert(TYPE_COMPACT, "S", "Shadow's client");
  insert(TYPE_COMPACT, "U", "UPnP NAT Bit Torrent");
  insert(TYPE_COMPACT, "O", "Osprey Permaseed");

  m_containers[TYPE_MAINLINE].reserve(5);

  insert(TYPE_MAINLINE, "M", "Mainline");
}

void
ClientInfo::insert(Type t, const char* key, const char* name) {
  if (t >= TYPE_MAXSIZE)
    throw torrent::input_error("Client info type out of range.");

  size_type keySize = sizeof_key(t);

  if (keySize != std::strlen(key))
    throw torrent::input_error("Client info key size not what was expected.");

  // Check if it is already present, not entirely optimal when
  // initializing but this only get run once.
  iterator itr = std::find_if(m_containers[t].begin(), m_containers[t].end(), client_info_equal(key, sizeof_key(t)));
  
  if (itr == m_containers[t].end())
    itr = m_containers[t].insert(m_containers[t].end(), value_type());

  std::memcpy(itr->first, key, keySize);
  itr->second = name;
}

char*
ClientInfo::print(char* first, char* last, const char* id) {

  if (id[0] == '-' && id[7] == '-' &&
      std::isalpha(id[1]) && std::isalpha(id[2]) &&
      std::isxdigit(id[3]) && std::isxdigit(id[4]) && std::isxdigit(id[5]) && std::isxdigit(id[6])) {
    // TYPE_AZUREUS.

    iterator itr = std::find_if(m_containers[TYPE_AZUREUS].begin(), m_containers[TYPE_AZUREUS].end(),
				client_info_equal(id + 1, sizeof_key(TYPE_AZUREUS)));

    if (itr != m_containers[TYPE_AZUREUS].end())
      first = print_buffer(first, last, "%s %hhu.%hhu.%hhu.%hhu", itr->second,
			   rak::hexchar_to_value(id[3]), rak::hexchar_to_value(id[4]),
			   rak::hexchar_to_value(id[5]), rak::hexchar_to_value(id[6]));
    
    else
      first = print_buffer(first, last, "unknown %c%c %hhu.%hhu.%hhu.%hhu", id[1], id[2],
			   rak::hexchar_to_value(id[3]), rak::hexchar_to_value(id[4]),
			   rak::hexchar_to_value(id[5]), rak::hexchar_to_value(id[6]));

  } else if (std::isalpha(id[0]) && id[4] == '-' &&
	     std::isxdigit(id[1]) && std::isxdigit(id[2]) && std::isxdigit(id[3])) {
    // TYPE_THREE_COMPACT.

    iterator itr = std::find_if(m_containers[TYPE_COMPACT].begin(), m_containers[TYPE_COMPACT].end(),
				client_info_equal(id, sizeof_key(TYPE_COMPACT)));

    if (itr != m_containers[TYPE_COMPACT].end())
      first = print_buffer(first, last, "%s %hhu.%hhu.%hhu", itr->second,
			   rak::hexchar_to_value(id[1]), rak::hexchar_to_value(id[2]), rak::hexchar_to_value(id[3]));
    
    else
      first = print_buffer(first, last, "unknown %c %hhu.%hhu.%hhu", id[0],
			   rak::hexchar_to_value(id[1]), rak::hexchar_to_value(id[2]), rak::hexchar_to_value(id[3]));
    
  } else if (std::isalpha(id[0]) && id[2] == '-' && id[4] == '-' && id[6] == '-' &&
	     std::isxdigit(id[1]) && std::isxdigit(id[3]) && std::isxdigit(id[5])) {
    // TYPE_THREE_SPARSE.

    iterator itr = std::find_if(m_containers[TYPE_MAINLINE].begin(), m_containers[TYPE_MAINLINE].end(),
				client_info_equal(id, sizeof_key(TYPE_MAINLINE)));

    if (itr != m_containers[TYPE_MAINLINE].end())
      first = print_buffer(first, last, "%s %hhu.%hhu.%hhu", itr->second,
			   rak::hexchar_to_value(id[1]), rak::hexchar_to_value(id[3]), rak::hexchar_to_value(id[5]));
    
    else
      first = print_buffer(first, last, "unknown %c %hhu.%hhu.%hhu", id[0],
			   rak::hexchar_to_value(id[1]), rak::hexchar_to_value(id[3]), rak::hexchar_to_value(id[5]));
    

  // And then the incompatible idiots that make life difficult for us
  // others. (There's '3' schemes to choose from already...)

  // Well... fuck this... I don't feel like adding the rest of the
  // checks as they wouldn't be possible to remove/modify.

  } else {
    first = print_buffer(first, last, "unknown");
  }

  return first;
}

}
