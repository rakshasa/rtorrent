#ifndef RTORRENT_CORE_DOWNLOAD_H
#define RTORRENT_CORE_DOWNLOAD_H

#include <torrent/common.h>
#include <torrent/download.h>
#include <torrent/download_info.h>
#include <torrent/hash_string.h>
#include <torrent/data/file_list.h>
#include <torrent/peer/connection_list.h>
#include <torrent/tracker/wrappers.h>

#include "globals.h"

namespace core {

class Download {
public:
  typedef torrent::Download             download_type;
  typedef torrent::FileList             file_list_type;
  typedef torrent::PeerList             peer_list_type;
  typedef torrent::TrackerList          tracker_list_type;
  typedef torrent::ConnectionList       connection_list_type;
  typedef download_type::ConnectionType connection_type;

  static const int variable_hashing_stopped = 0;
  static const int variable_hashing_initial = 1;
  static const int variable_hashing_last    = 2;
  static const int variable_hashing_rehash  = 3;

  Download(download_type d);
  ~Download();

  auto                info() const                             { return m_download.info(); }
  auto                data() const                             { return m_download.data(); }

  auto                 main()                                  { return m_download.main(); }

  bool                is_open() const                          { return m_download.info()->is_open(); }
  bool                is_active() const                        { return m_download.info()->is_active(); }
  bool                is_done() const                          { return m_download.file_list()->is_done(); }
  bool                is_downloading() const                   { return is_active() && !is_done(); }
  bool                is_seeding() const                       { return is_active() && is_done(); }

  // FIXME: Fixed a bug in libtorrent that caused is_hash_checked to
  // return true when the torrent is closed. Remove this redundant
  // test in the next milestone.
  bool                is_hash_checked() const                  { return is_open() && m_download.is_hash_checked(); }
  bool                is_hash_checking() const                 { return m_download.is_hash_checking(); }

  bool                is_hash_failed() const                   { return m_hashFailed; }
  void                set_hash_failed(bool v)                  { m_hashFailed = v; }

  download_type*       download()                              { return &m_download; }
  const download_type* c_download() const                      { return &m_download; }

  file_list_type*       file_list()                            { return m_download.file_list(); }
  const file_list_type* c_file_list() const                    { return m_download.file_list(); }

  peer_list_type*       peer_list()                            { return m_download.peer_list(); }
  const peer_list_type* c_peer_list() const                    { return m_download.peer_list(); }

  torrent::Object*    bencode()                                { return m_download.bencode(); }

  auto                tracker_controller()                     { return m_download.tracker_controller(); }
  uint32_t            tracker_list_size() const                { return m_download.c_tracker_controller().size(); }

  auto                connection_list()                        { return m_download.connection_list(); }
  uint32_t            connection_list_size() const;

  const std::string&  message() const                          { return m_message; }
  void                set_message(const std::string& msg)      { m_message = msg; }

  void                enable_udp_trackers(bool state);

  uint32_t            priority();
  void                set_priority(uint32_t p);

  uint32_t            resume_flags()                           { return m_resumeFlags; }
  void                set_resume_flags(uint32_t flags)         { m_resumeFlags = flags; }

  void                set_root_directory(const std::string& path);

  void                set_throttle_name(const std::string& throttleName);

  bool                operator == (const std::string& str) const;

  float               distributed_copies() const;

  // HACK: Choke group setting.
  unsigned int        group() const { return m_group; }
  void                set_group(unsigned int g) { m_group = g; }

private:
  Download(const Download&);
  void operator () (const Download&);

  void                receive_tracker_msg(std::string msg);

  void                receive_chunk_failed(uint32_t idx);

  // Store the FileList instance so we can use slots etc on it.
  download_type       m_download;
  bool                m_hashFailed{};
  std::string         m_message;
  uint32_t            m_resumeFlags{~uint32_t{}};
  unsigned int        m_group{};
};

inline bool
Download::operator == (const std::string& str) const {
  return str.size() == torrent::HashString::size_data && *torrent::HashString::cast_from(str) == m_download.info()->hash();
}

}

#endif
