#include "config.h"

#include <stdexcept>
#include <cstring>
#include <fstream>
#include <istream>
#include <sigc++/bind.h>
#include <torrent/exceptions.h>
#include <torrent/http.h>
#include <torrent/torrent.h>

#include "manager.h"

namespace core {

void
Manager::initialize() {
  torrent::Http::set_factory(m_poll.get_http_factory());
  m_httpQueue.slot_factory(m_poll.get_http_factory());

  CurlStack::init();

  torrent::initialize();
  torrent::listen_open(10000, 20000);
}

void
Manager::cleanup() {
  torrent::cleanup();
  core::CurlStack::cleanup();
}

void
Manager::insert(const std::string& uri) {
  if (std::strncmp(uri.c_str(), "http://", 7))
    create_file(uri);
  else
    create_http(uri);
}

void
Manager::erase(DownloadList::iterator itr) {
  if (itr->get_download().is_active())
    throw std::logic_error("core::Manager::erase(...) called on an active download");

  if (itr->get_download().is_open())
    itr->close();

  m_downloadList.erase(itr);
}  

void
Manager::start(Download* d) {
  if (d->get_download().is_active())
    return;

  if (!d->get_download().is_open())
    d->open();

  if (d->get_download().is_hash_checked())
    d->start();
  else
    m_hashQueue.insert(d, sigc::mem_fun(*d, &Download::start));
}

void
Manager::stop(Download* d) {
  m_hashQueue.remove(d);

  d->stop();
}

void
Manager::create_file(const std::string& uri) {
  DownloadList::iterator itr = m_downloadList.end();

  try {
    std::fstream f(uri.c_str(), std::ios::in);
    
    itr = m_downloadList.insert(&f);

    start(&*itr);

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

    start(&*itr);

  } catch (torrent::local_error& e) {
    // What to do? Keep in list for now.
  }
}  

}
