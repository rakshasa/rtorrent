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

#ifndef RTORRENT_CORE_MANAGER_H
#define RTORRENT_CORE_MANAGER_H

#include <iosfwd>
#include <vector>

#include <torrent/utils/log_buffer.h>
#include <torrent/connection_manager.h>

#include "download_list.h"
#include "poll_manager.h"
#include "range_map.h"

namespace torrent {
  class Bencode;
}

namespace utils {
class FileStatusCache;
}

namespace core {

class DownloadStore;
class HttpQueue;

typedef std::map<std::string, torrent::ThrottlePair> ThrottleMap;

class View;

class Manager {
public:
  typedef DownloadList::iterator                    DListItr;
  typedef utils::FileStatusCache                    FileStatusCache;

  // typedef std::tr1::function<void (DownloadList::iterator)> slot_ready;
  // typedef std::tr1::function<void ()>                       slot_void;

  Manager();
  ~Manager();

  DownloadList*       download_list()                     { return m_downloadList; }
  DownloadStore*      download_store()                    { return m_downloadStore; }
  FileStatusCache*    file_status_cache()                 { return m_fileStatusCache; }

  HttpQueue*          http_queue()                        { return m_httpQueue; }
  CurlStack*          http_stack()                        { return m_httpStack; }

  View*               hashing_view()                      { return m_hashingView; }
  void                set_hashing_view(View* v);

  torrent::log_buffer* log_important()                    { return m_log_important; }
  torrent::log_buffer* log_complete()                     { return m_log_complete; }

  ThrottleMap&          throttles()                       { return m_throttles; }
  torrent::ThrottlePair get_throttle(const std::string& name);

  // Use custom throttle for the given range of IP addresses.
  void                  set_address_throttle(uint32_t begin, uint32_t end, torrent::ThrottlePair throttles);
  torrent::ThrottlePair get_address_throttle(const sockaddr* addr);

  // Really should find a more descriptive name.
  void                initialize_second();
  void                cleanup();

  void                listen_open();

  std::string         bind_address() const;
  void                set_bind_address(const std::string& addr);

  std::string         local_address() const;
  void                set_local_address(const std::string& addr);

  std::string         proxy_address() const;
  void                set_proxy_address(const std::string& addr);

  void                shutdown(bool force);

  void                push_log(const char* msg);
  void                push_log_std(const std::string& msg) { m_log_important->lock_and_push_log(msg.c_str(), msg.size(), 0); m_log_complete->lock_and_push_log(msg.c_str(), msg.size(), 0); }
  void                push_log_complete(const std::string& msg) { m_log_complete->lock_and_push_log(msg.c_str(), msg.size(), 0); }

  void                handshake_log(const sockaddr* sa, int msg, int err, const torrent::HashString* hash);

  static const int create_start    = 0x1;
  static const int create_tied     = 0x2;
  static const int create_quiet    = 0x4;
  static const int create_raw_data = 0x8;

  typedef std::vector<std::string> command_list_type;

  // Temporary, find a better place for this.
  void                try_create_download(const std::string& uri, int flags, const command_list_type& commands);
  void                try_create_download_expand(const std::string& uri, int flags, command_list_type commands = command_list_type());
  void                try_create_download_from_meta_download(torrent::Object* bencode, const std::string& metafile);

private:
  typedef RangeMap<uint32_t, torrent::ThrottlePair> AddressThrottleMap;

  void                create_http(const std::string& uri);
  void                create_final(std::istream* s);

  void                initialize_bencode(Download* d);

  void                receive_http_failed(std::string msg);
  void                receive_hashing_changed();

  DownloadList*       m_downloadList;
  DownloadStore*      m_downloadStore;
  FileStatusCache*    m_fileStatusCache;
  HttpQueue*          m_httpQueue;
  CurlStack*          m_httpStack;

  View*               m_hashingView;

  ThrottleMap         m_throttles;
  AddressThrottleMap  m_addressThrottles;

  torrent::log_buffer* m_log_important;
  torrent::log_buffer* m_log_complete;
};

// Meh, cleanup.
extern void receive_tracker_dump(const std::string& url, const char* data, size_t size);

}

#endif
