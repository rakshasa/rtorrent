#ifndef RTORRENT_RPC_XMLRPC_H
#define RTORRENT_RPC_XMLRPC_H

#include <functional>
#include <torrent/common.h>
#include <torrent/hash_string.h>
#include <torrent/tracker/tracker.h>

#include "command.h"
#include "scgi_task.h"

namespace core {
  class Download;
}

namespace rpc {

class XmlRpc {
public:
  typedef std::function<core::Download* (const char*)>                                slot_download;
  typedef std::function<torrent::File* (core::Download*, uint32_t)>                   slot_file;
  typedef std::function<torrent::tracker::Tracker (core::Download*, uint32_t)>        slot_tracker;
  typedef std::function<torrent::Peer* (core::Download*, const torrent::HashString&)> slot_peer;
  typedef std::function<bool (const char*, uint32_t)>                                 slot_write;

  static const int dialect_generic = 0;
  static const int dialect_i8      = 1;
  static const int dialect_apache  = 2;

  // These need to match CommandMap type values.
  static const int call_generic    = 0;
  static const int call_any        = 1;
  static const int call_download   = 2;
  static const int call_peer       = 3;
  static const int call_tracker    = 4;
  static const int call_file       = 5;
  static const int call_file_itr   = 6;

  static void object_to_target(const torrent::Object& obj, int callFlags, rpc::target_type* target);

  bool                is_valid() const;

  void                initialize();
  void                cleanup();

  bool                process(const char* inBuffer, uint32_t length, slot_write slotWrite);

  void                insert_command(const char* name, const char* parm, const char* doc);

  int                 dialect() { return m_dialect; }
  void                set_dialect(int dialect);

  slot_download&      slot_find_download() { return m_slotFindDownload; }
  slot_file&          slot_find_file()     { return m_slotFindFile; }
  slot_tracker&       slot_find_tracker()  { return m_slotFindTracker; }
  slot_peer&          slot_find_peer()     { return m_slotFindPeer; }

  int64_t             size_limit();
  void                set_size_limit(uint64_t size);

private:
  static const char*  store_command_name(const char* name);

  static std::vector<std::unique_ptr<const char>> m_command_names;

  slot_download       m_slotFindDownload;
  slot_file           m_slotFindFile;
  slot_tracker        m_slotFindTracker;
  slot_peer           m_slotFindPeer;

  // Only used by xmlrpc-c
  void*               m_env{};
  void*               m_registry{};

  int                 m_dialect{dialect_i8};

  // Only used by tinyxml2
  bool                m_isValid;
  uint64_t            m_sizeLimit{SCgiTask::max_content_size};
};

}

#endif
