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

#ifndef RTORRENT_CORE_DOWNLOAD_H
#define RTORRENT_CORE_DOWNLOAD_H

#include <sigc++/connection.h>
#include <torrent/download.h>
#include <torrent/hash_string.h>
#include <torrent/tracker_list.h>
#include <torrent/data/file_list.h>

#include "globals.h"

namespace torrent {
  class TrackerList;
}

namespace core {

class Download {
public:
  typedef torrent::Download             download_type;
  typedef torrent::FileList             file_list_type;
  typedef torrent::TrackerList          tracker_list_type;
  typedef torrent::ConnectionList       connection_list_type;
  typedef download_type::ConnectionType connection_type;

  static const int variable_hashing_stopped = 0;
  static const int variable_hashing_initial = 1;
  static const int variable_hashing_last    = 2;
  static const int variable_hashing_rehash  = 3;

  Download(download_type d);
  ~Download();

  bool                is_open() const                          { return m_download.is_open(); }
  bool                is_active() const                        { return m_download.is_active(); }
  bool                is_done() const                          { return m_download.file_list()->is_done(); }
  bool                is_downloading() const                   { return is_active() && !is_done(); }
  bool                is_seeding() const                       { return is_active() && is_done(); }

  // FIXME: Fixed a bug in libtorrent that caused is_hash_checked to
  // return true when the torrent is closed. Remove this redundant
  // test in the next milestone.
  bool                is_hash_checked() const                  { return m_download.is_open() && m_download.is_hash_checked(); }
  bool                is_hash_checking() const                 { return m_download.is_hash_checking(); }

  bool                is_hash_failed() const                   { return m_hashFailed; }
  void                set_hash_failed(bool v)                  { m_hashFailed = v; }

  download_type*       download()                              { return &m_download; }
  const download_type* c_download() const                      { return &m_download; }

  file_list_type*       file_list()                            { return m_download.file_list(); }
  const file_list_type* c_file_list() const                    { return m_download.file_list(); }

  torrent::Object*    bencode()                                { return m_download.bencode(); }

  tracker_list_type*  tracker_list()                           { return m_download.tracker_list(); }
  uint32_t            tracker_list_size() const                { return m_download.tracker_list()->size(); }

  connection_list_type* connection_list()                      { return m_download.connection_list(); }

  const std::string&  message() const                          { return m_message; }
  void                set_message(const std::string& msg)      { m_message = msg; }

  uint32_t            chunks_failed() const                    { return m_chunksFailed; }

  void                enable_udp_trackers(bool state);

  uint32_t            priority();
  void                set_priority(uint32_t p);

  uint32_t            resume_flags()                           { return m_resumeFlags; }
  void                set_resume_flags(uint32_t flags)         { m_resumeFlags = flags; }

  void                set_root_directory(const std::string& path);

  bool                operator == (const std::string& str) const;

  float               distributed_copies() const;

private:
  Download(const Download&);
  void operator () (const Download&);

  void                receive_tracker_msg(std::string msg);
  void                receive_storage_error(std::string msg);

  void                receive_chunk_failed(uint32_t idx);

  // Store the FileList instance so we can use slots etc on it.
  download_type       m_download;

  bool                m_hashFailed;

  std::string         m_message;
  uint32_t            m_chunksFailed;

  uint32_t            m_resumeFlags;

  sigc::connection    m_connTrackerSucceded;
  sigc::connection    m_connTrackerFailed;
  sigc::connection    m_connStorageError;
};

inline bool
Download::operator == (const std::string& str) const {
  return str.size() == torrent::HashString::size_data && *torrent::HashString::cast_from(str) == m_download.info_hash();
}

}

#endif
