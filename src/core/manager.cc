// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#include "config.h"

#include <stdexcept>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <istream>
#include <sstream>
#include <iterator>
#include <sigc++/bind.h>
#include <torrent/bencode.h>
#include <torrent/exceptions.h>
#include <torrent/torrent.h>

#include "download.h"
#include "manager.h"
#include "curl_get.h"

namespace core {

void
Manager::initialize() {
  torrent::Http::set_factory(m_poll.get_http_factory());
  m_httpQueue.slot_factory(m_poll.get_http_factory());

  CurlStack::init();

  torrent::initialize();

  if (!torrent::listen_open(m_portFirst, m_portLast, m_listenIp))
    throw std::runtime_error("Could not open port for listening.");

  // Register log signals.
}

void
Manager::cleanup() {
  // Need to disconnect log signals? Not really since we won't receive
  // any more.

  torrent::cleanup();
  core::CurlStack::cleanup();
}

void
Manager::insert(std::string uri) {
  if (std::strncmp(uri.c_str(), "http://", 7))
    create_file(uri);
  else
    create_http(uri);
}

Manager::iterator
Manager::erase(DownloadList::iterator itr) {
  if ((*itr)->get_download().is_active())
    throw std::logic_error("core::Manager::erase(...) called on an active download");

  if (!(*itr)->get_download().is_open())
    throw std::logic_error("core::Manager::erase(...) called on an closed download");

  m_downloadStore.remove(*itr);

  return m_downloadList.erase(itr);
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
    // This can cause infinit loops.
    m_hashQueue.insert(d, sigc::mem_fun(d, &Download::start));
}

void
Manager::stop(Download* d) {
  m_hashQueue.remove(d);

  d->stop();

  d->get_download().hash_save();
  m_downloadStore.save(d);
}

void
Manager::set_dns(const std::string& dns) {
  unsigned int a, b, c, d;

  if (std::sscanf(dns.c_str(), "%i.%i.%i.%i", &a, &b, &c, &d) != 4 ||
      (a >= 256 || b >= 256 || c >= 256 || d >= 256))
    throw std::runtime_error("Tried to set invalid ip address.");

  std::stringstream str;
  str << a << '.' << b << '.' << c << '.' << d;

  m_dns = str.str();
}

void
Manager::receive_http_done(CurlGet* http) {
  try {
    create_final(http->get_stream());

  } catch (torrent::local_error& e) {
    // What to do? Keep in list for now.
    m_logImportant.push_front(e.what());
    m_logComplete.push_front(e.what());
  }
}  

void
Manager::receive_http_failed(std::string msg) {
  m_logImportant.push_front("Http download error: \"" + msg + "\"");
  m_logComplete.push_front("Http download error: \"" + msg + "\"");
}

void
Manager::create_file(const std::string& uri) {
  try {
    std::fstream f(uri.c_str(), std::ios::in);
    
    create_final(&f);

  } catch (torrent::local_error& e) {
    // What to do? Keep in list for now.
    m_logImportant.push_front(e.what());
    m_logComplete.push_front(e.what());
  }
}

void
Manager::create_http(const std::string& uri) {
  core::HttpQueue::iterator itr = m_httpQueue.insert(uri);

  (*itr)->signal_done().slots().push_front(sigc::bind(sigc::mem_fun(*this, &core::Manager::receive_http_done), *itr));
  (*itr)->signal_failed().slots().push_front(sigc::mem_fun(*this, &core::Manager::receive_http_failed));
}

Manager::iterator
Manager::create_final(std::istream* s) {
  iterator itr = m_downloadList.insert(s);
  
  (*itr)->get_download().set_ip(m_dns);

  if (!m_defaultRoot.empty())
    (*itr)->get_download().set_root_dir(m_defaultRoot + ((*itr)->get_download().get_entry_size() ? (*itr)->get_download().get_name() : ""));

  start(*itr);
  m_downloadStore.save(*itr);

  if (m_debugTracker >= 0)
    (*itr)->get_download().signal_tracker_dump(sigc::mem_fun(*this, &Manager::receive_debug_tracker));

  // If we want to monitor network stuff.
  (*itr)->get_download().signal_network_log(sigc::mem_fun(m_logComplete, &Log::push_front));

  return itr;
}

void
Manager::receive_debug_tracker(torrent::Bencode& bencode) {
  std::stringstream filename;
  filename << "./tracker_dump." << m_debugTracker++;

  std::fstream out(filename.str().c_str(), std::ios::out | std::ios::trunc);

  if (!out.good())
    return;

  out << bencode;
}

}
