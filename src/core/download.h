#ifndef RTORRENT_CORE_DOWNLOAD_H
#define RTORRENT_CORE_DOWNLOAD_H

#include <sigc++/connection.h>
#include <torrent/download.h>

namespace core {

class Download {
public:
  void               set_download(torrent::Download d);
  void               release_download();

  torrent::Download& get_download()                  { return m_download; }
  std::string        get_hash()                      { return m_download.get_hash(); }
  
  const std::string& get_tracker_msg()               { return m_trackerMsg; }

  void               open()                          { m_download.open(); }
  void               close()                         { m_download.close(); }

  void               stop()                          { m_download.stop(); }

  void               hash_check(bool resume = false) { m_download.hash_check(resume); }

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
