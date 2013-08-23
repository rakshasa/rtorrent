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

// The DownloadFactory class assures that loading torrents can be done
// anywhere in the code by queueing the task. The user may change
// settings while, or even after, the torrent is loading.

#ifndef RTORRENT_CORE_DOWNLOAD_FACTORY_H
#define RTORRENT_CORE_DOWNLOAD_FACTORY_H

#include <iosfwd>
#include <rak/priority_queue_default.h>
#include <torrent/object.h>
#include <tr1/functional>

#include "http_queue.h"

namespace core {

class Manager;

class DownloadFactory {
public:
  typedef std::tr1::function<void ()> slot_void;
  typedef std::vector<std::string> command_list_type;

  // Do not destroy this object while it is in a HttpQueue.
  DownloadFactory(Manager* m);
  ~DownloadFactory();

  // Calling of receive_load() is delayed so you can change whatever
  // you want without fear of the slots being triggered as you call
  // load() or commit().
  void                load(const std::string& uri);
  void                load_raw_data(const std::string& input);
  void                commit();

  command_list_type&         commands()     { return m_commands; }
  torrent::Object::map_type& variables()    { return m_variables; }

  bool                get_session() const   { return m_session; }
  void                set_session(bool v)   { m_session = v; }

  bool                get_start() const     { return m_start; }
  void                set_start(bool v)     { m_start = v; }

  bool                print_log() const     { return m_printLog; }
  void                set_print_log(bool v) { m_printLog = v; }

  void                slot_finished(slot_void s) { m_slot_finished = s; }

private:
  void                receive_load();
  void                receive_loaded();
  void                receive_commit();
  void                receive_success();
  void                receive_failed(const std::string& msg);

  void                log_created(Download* download, torrent::Object* rtorrent);

  void                initialize_rtorrent(Download* download, torrent::Object* rtorrent);

  Manager*            m_manager;
  std::iostream*      m_stream;
  torrent::Object*    m_object;

  bool                m_commited;
  bool                m_loaded;

  std::string         m_uri;
  bool                m_session;
  bool                m_start;
  bool                m_printLog;
  bool                m_isFile;

  command_list_type         m_commands;
  torrent::Object::map_type m_variables;

  slot_void           m_slot_finished;
  rak::priority_item  m_taskLoad;
  rak::priority_item  m_taskCommit;
};

bool is_network_uri(const std::string& uri);
bool is_magnet_uri(const std::string& uri);

}

#endif
