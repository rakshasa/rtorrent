#ifndef RTORRENT_CORE_MANAGER_H
#define RTORRENT_CORE_MANAGER_H

#include "download_list.h"
#include "hash_queue.h"
#include "http_queue.h"
#include "poll.h"

namespace core {

class Manager {
public:
  typedef sigc::slot1<void, DownloadList::iterator> SlotReady;
  typedef sigc::slot0<void>                         SlotFailed;

  DownloadList& get_download_list() { return m_downloadList; }
  HashQueue&    get_hash_queue()    { return m_hashQueue; }
  HttpQueue&    get_http_queue()    { return m_httpQueue; }

  Poll&         get_poll()          { return m_poll; }

  void          initialize();
  void          cleanup();

  void          insert(const std::string& uri);

  void          start(Download* d);
  void          stop(Download* d);

private:
  void          receive_http_done(torrent::Http* http);

  void          create_file(const std::string& uri);
  void          create_http(const std::string& uri);

  DownloadList  m_downloadList;
  HashQueue     m_hashQueue;
  HttpQueue     m_httpQueue;
  Poll          m_poll;
};

}

#endif
