#ifndef RTORRENT_CORE_MANAGER_H
#define RTORRENT_CORE_MANAGER_H

#include "download_list.h"
#include "http_queue.h"

namespace core {

class Manager {
public:
  typedef sigc::slot1<void, DownloadList::iterator> SlotReady;
  typedef sigc::slot0<void>                         SlotFailed;

  DownloadList& get_download_list() { return m_downloadList; }
  HttpQueue&    get_http_queue()    { return m_httpQueue; }

  void          insert(const std::string& uri);

private:
  void          receive_http_done(torrent::Http* http);

  void          create_file(const std::string& uri);
  void          create_http(const std::string& uri);

  DownloadList  m_downloadList;
  HttpQueue     m_httpQueue;
};

}

#endif
