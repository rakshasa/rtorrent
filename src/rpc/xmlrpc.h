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

#ifndef RTORRENT_RPC_XMLRPC_H
#define RTORRENT_RPC_XMLRPC_H

#include <rak/functional_fun.h>

typedef struct _xmlrpc_env xmlrpc_env;
typedef struct _xmlrpc_registry xmlrpc_registry;

namespace torrent {
  class Object;
}

namespace rpc {

class XmlRpc {
public:
  typedef rak::function2<bool, const char*, uint32_t> slot_write;
  typedef rak::function2<const torrent::Object&, const char*, const torrent::Object&> slot_call_command;

  XmlRpc();
  ~XmlRpc();

  bool                process(const char* inBuffer, uint32_t length, slot_write slotWrite);

  void                set_slot_call_command_get(slot_call_command::base_type* s) { m_slotGet.set(s); }
  void                set_slot_call_command_set(slot_call_command::base_type* s) { m_slotSet.set(s); }

private:
  xmlrpc_env*         m_env;
  xmlrpc_registry*    m_registry;

  slot_call_command   m_slotGet;
  slot_call_command   m_slotSet;
};

}

#endif
