#ifndef RTORRENT_CORE_DOWNLOAD_H
#define RTORRENT_CORE_DOWNLOAD_H

#include <sigc++/connection.h>
#include <torrent/download.h>

namespace core {

class Download {
public:
  bool               is_open()                       { return m_download.is_open(); }
  bool               is_done()                       { return m_download.get_chunks_done() == m_download.get_chunks_total(); }

  void               set_download(torrent::Download d);
  void               release_download();

  torrent::Download& get_download()                  { return m_download; }
  std::string        get_hash()                      { return m_download.get_hash(); }
  
  const std::string& get_tracker_msg()               { return m_trackerMsg; }

  void               start()                         { m_download.start(); }
  void               stop()                          { m_download.stop(); }

  void               open()                          { m_download.open(); }
  void               close()                         { m_download.close(); }

  bool operator == (const std::string& str)          { return str == m_download.get_hash(); }

private:
  void               receive_tracker_msg(std::string msg);

  torrent::Download  m_download;

  std::string        m_trackerMsg;

  sigc::connection   m_connTrackerSucceded;
  sigc::connection   m_connTrackerFailed;
};

}

#endif
