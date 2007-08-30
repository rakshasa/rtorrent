// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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

namespace core {
  class Download;
}

namespace torrent {
  class File;
  class Object;
  class Tracker;
}

namespace rpc {

class XmlRpc {
public:
  typedef rak::function1<core::Download*, const char*>                 slot_find_download;
  typedef rak::function2<torrent::File*, core::Download*, uint32_t>    slot_find_file;
  typedef rak::function2<torrent::Tracker*, core::Download*, uint32_t> slot_find_tracker;
  typedef rak::function2<bool, const char*, uint32_t>                  slot_write;

  static const int dialect_generic = 0;
  static const int dialect_i8      = 1;
  static const int dialect_apache  = 2;

  // These need to match CommandMap type values.
  static const int call_generic    = 0;
  static const int call_download   = 1;
  static const int call_peer       = 2;
  static const int call_tracker    = 3;
  static const int call_file       = 4;
  static const int call_file_itr   = 5;

  XmlRpc() : m_env(NULL), m_registry(NULL), m_dialect(dialect_i8) {}

  bool                is_valid() const { return m_env != NULL; }

  void                initialize();
  void                cleanup();

  bool                process(const char* inBuffer, uint32_t length, slot_write slotWrite);

  void                insert_command(const char* name, const char* parm, const char* doc);

  int                 dialect() { return m_dialect; }
  void                set_dialect(int dialect);

  slot_find_download& get_slot_find_download()                                    { return m_slotFindDownload; }
  void                set_slot_find_download(slot_find_download::base_type* slot) { m_slotFindDownload.set(slot); }

  slot_find_file&     get_slot_find_file()                                        { return m_slotFindFile; }
  void                set_slot_find_file(slot_find_file::base_type* slot)         { m_slotFindFile.set(slot); }

  slot_find_tracker&  get_slot_find_tracker()                                     { return m_slotFindTracker; }
  void                set_slot_find_tracker(slot_find_tracker::base_type* slot)   { m_slotFindTracker.set(slot); }

private:
  void*               m_env;
  void*               m_registry;

  int                 m_dialect;

  slot_find_download  m_slotFindDownload;
  slot_find_file      m_slotFindFile;
  slot_find_tracker   m_slotFindTracker;
};

}

#endif
