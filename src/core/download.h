#ifndef RTORRENT_CORE_DOWNLOAD_H
#define RTORRENT_CORE_DOWNLOAD_H

#include <sigc++/connection.h>
#include <torrent/download.h>

namespace core {

class Download {
public:
  Download(torrent::Download d) : m_download(d) {}
  
  std::string        get_hash()                      { return m_download.get_hash(); }
  torrent::Download& get_download()                  { return m_download; }

  void               open()                          { m_download.open(); }
  void               close()                         { m_download.close(); }

  void               stop()                          { m_download.stop(); }

  void               hash_check(bool resume = false) { m_download.hash_check(resume); }

  bool operator == (const std::string& str)          { return str == m_download.get_hash(); }

private:
  torrent::Download  m_download;
};

}

#endif
