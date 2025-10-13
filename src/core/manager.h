#ifndef RTORRENT_CORE_MANAGER_H
#define RTORRENT_CORE_MANAGER_H

#include <iosfwd>
#include <memory>
#include <vector>
#include <torrent/utils/log_buffer.h>
#include <torrent/connection_manager.h>
#include <torrent/object.h>

#include "download_list.h"
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

  Manager();
  ~Manager();

  DownloadList*       download_list()                     { return m_download_list.get(); }
  DownloadStore*      download_store()                    { return m_download_store.get(); }
  FileStatusCache*    file_status_cache()                 { return m_file_status_cache.get(); }

  HttpQueue*          http_queue()                        { return m_http_queue.get(); }

  View*               hashing_view()                      { return m_hashingView; }
  void                set_hashing_view(View* v);

  torrent::log_buffer* log_important()                    { return m_log_important.get(); }
  torrent::log_buffer* log_complete()                     { return m_log_complete.get(); }

  ThrottleMap&          throttles()                       { return m_throttles; }
  torrent::ThrottlePair get_throttle(const std::string& name);

  int64_t             retrieve_throttle_value(const torrent::Object::string_type& name, bool rate, bool up);

  // Use custom throttle for the given range of IP addresses.
  void                  set_address_throttle(uint32_t begin, uint32_t end, torrent::ThrottlePair throttles);
  torrent::ThrottlePair get_address_throttle(const sockaddr* addr);

  void                cleanup();

  void                listen_open();

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

  std::unique_ptr<DownloadList>    m_download_list;
  std::unique_ptr<DownloadStore>   m_download_store;
  std::unique_ptr<FileStatusCache> m_file_status_cache;
  std::unique_ptr<HttpQueue>       m_http_queue;

  View*               m_hashingView{};

  ThrottleMap         m_throttles;
  AddressThrottleMap  m_addressThrottles;

  torrent::log_buffer_ptr m_log_important;
  torrent::log_buffer_ptr m_log_complete;
};

// Meh, cleanup.
extern void receive_tracker_dump(const std::string& url, const char* data, size_t size);

}

#endif
