#include "config.h"

#include <cstring>
#include <fstream>
#include <istream>
#include <sigc++/bind.h>
#include <torrent/exceptions.h>
#include <torrent/http.h>

#include "manager.h"


namespace core {

void
Manager::insert(const std::string& uri) {
  if (std::strncmp(uri.c_str(), "http://", 7))
    create_file(uri);
  else
    create_http(uri);
}

void
Manager::create_file(const std::string& uri) {
  DownloadList::iterator itr = m_downloadList.end();

  try {
    std::fstream f(uri.c_str(), std::ios::in);
    
    itr = m_downloadList.insert(&f);

    itr->open();
    itr->hash_check();

  } catch (torrent::local_error& e) {
    // What to do? Keep in list for now.
  }
}

void
Manager::create_http(const std::string& uri) {
  core::HttpQueue::iterator itr = m_httpQueue.insert(uri);

  (*itr)->signal_done().slots().push_front(sigc::bind(sigc::mem_fun(*this, &core::Manager::receive_http_done), *itr));
  // Add the failed signal here.
}

void
Manager::receive_http_done(torrent::Http* http) {
  DownloadList::iterator itr = m_downloadList.end();

  try {
    itr = m_downloadList.insert(http->get_stream());

    itr->open();
    itr->hash_check();

  } catch (torrent::local_error& e) {
    // What to do? Keep in list for now.
  }
}  

}
