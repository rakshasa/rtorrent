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
typedef struct _xmlrpc_value xmlrpc_value;
typedef struct _xmlrpc_registry xmlrpc_registry;

namespace core {
  class Download;
}

namespace torrent {
  class Object;
}

namespace rpc {

class XmlRpc {
public:
  typedef rak::function1<core::Download*, const char*> slot_find_download;
  typedef rak::function2<bool, const char*, uint32_t>  slot_write;

  XmlRpc();
  ~XmlRpc();

  bool                process(const char* inBuffer, uint32_t length, slot_write slotWrite);

  void                insert_command(const char* name, const char* parm, const char* doc, bool onDownload);

  static void         set_slot_find_download(slot_find_download::base_type* slot) { m_slotFindDownload.set(slot); }

  static xmlrpc_value* call_command(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo);
  static xmlrpc_value* call_command_d(xmlrpc_env* env, xmlrpc_value* args, void* voidServerInfo);

private:
  static core::Download* xmlrpc_to_download(xmlrpc_env* env, xmlrpc_value* value);
  static torrent::Object xmlrpc_to_object_d(xmlrpc_env* env, xmlrpc_value* value, core::Download** download);

  xmlrpc_env*         m_env;
  xmlrpc_registry*    m_registry;

  static slot_find_download m_slotFindDownload;
};

}

#endif
