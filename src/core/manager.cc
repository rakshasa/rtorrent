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
// In addition, as a special exception, the copyright holders give
// permission to link the code of portions of this program with the
// OpenSSL library under certain conditions as described in each
// individual source file, and distribute linked combinations
// including the two.
//
// You must obey the GNU General Public License in all respects for
// all of the code used other than OpenSSL.  If you modify file(s)
// with this exception, you may extend this exception to your version
// of the file(s), but you are not obligated to do so.  If you do not
// wish to do so, delete this exception statement from your version.
// If you delete this exception statement from all source files in the
// program, then also delete it here.
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
#include <unistd.h>
#include <sys/select.h>
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <torrent/bencode.h>
#include <torrent/exceptions.h>

#include "curl_get.h"
#include "download.h"
#include "manager.h"
#include "poll_manager_epoll.h"
#include "poll_manager_select.h"

namespace core {

static void
connect_signal_network_log(Download* d, torrent::Download::SlotString s) {
  d->get_download().signal_network_log(s);
}

static void
connect_signal_tracker_log(Download* d, torrent::Download::SlotString s) {
  d->get_download().signal_tracker_failed(s);
}

Manager::Manager() :
  m_portRandom(false),
  m_portFirst(6890),
  m_portLast(6999),
  m_checkHash(true) {

  // Consider doing this somewhere else.
  if ((m_pollManager = PollManagerEPoll::create(sysconf(_SC_OPEN_MAX))) != NULL)
    m_logImportant.push_front("Using 'epoll' based polling");
  else if ((m_pollManager = PollManagerSelect::create(sysconf(_SC_OPEN_MAX))) != NULL)
    m_logImportant.push_front("Using 'select' based polling");
  else
    throw std::runtime_error("Could not create any PollManager");
}

Manager::~Manager() {
  delete m_pollManager;
}

void
Manager::initialize() {
  torrent::Http::set_factory(m_pollManager->get_http_stack()->get_http_factory());
  m_httpQueue.slot_factory(m_pollManager->get_http_stack()->get_http_factory());

  CurlStack::global_init();
  listen_open();

  if (torrent::get_max_open_files() + torrent::get_max_open_sockets() + 32 > FD_SETSIZE) {
    m_logImportant.push_front("Warning: Max open sockets and files exceeds FD_SETSIZE");
    m_logComplete.push_front("Warning: Max open sockets and files exceeds FD_SETSIZE");
  }    

  if (torrent::get_max_open_files() + torrent::get_max_open_sockets() + 32 > (unsigned int)sysconf(_SC_OPEN_MAX)) {
    m_logImportant.push_front("Warning: Max open sockets and files exceeds _SC_OPEN_MAX");
    m_logComplete.push_front("Warning: Max open sockets and files exceeds _SC_OPEN_MAX");
  }

  // Register slots to be called when a download is inserted/erased,
  // opened or closed.
  m_downloadList.slot_map_insert()["0_initialize_bencode"]  = sigc::mem_fun(*this, &Manager::initialize_bencode);
  m_downloadList.slot_map_insert()["1_connect_network_log"] = sigc::bind(sigc::ptr_fun(&connect_signal_network_log), sigc::mem_fun(m_logComplete, &Log::push_front));
  m_downloadList.slot_map_insert()["1_connect_tracker_log"] = sigc::bind(sigc::ptr_fun(&connect_signal_tracker_log), sigc::mem_fun(m_logComplete, &Log::push_front));
  //m_downloadList.slot_map_insert()["1_enable_udp_trackers"] = sigc::bind(sigc::mem_fun(&core::Download::enable_udp_trackers), true);

  m_downloadList.slot_map_erase()["1_hash_queue_remove"]    = sigc::mem_fun(m_hashQueue, &HashQueue::remove);
  m_downloadList.slot_map_erase()["1_store_remove"]         = sigc::mem_fun(m_downloadStore, &DownloadStore::remove);

  m_downloadList.slot_map_open()["1_download_open"]         = sigc::mem_fun(&Download::call<void, &torrent::Download::open>);

  // Currently does not call stop, might want to add a function that
  // checks if we're running, and if so stop?
  m_downloadList.slot_map_close()["1_hash_queue_remove"]    = sigc::mem_fun(m_hashQueue, &HashQueue::remove);
  m_downloadList.slot_map_close()["2_download_close"]       = sigc::mem_fun(&Download::call<void, &torrent::Download::close>);

  m_downloadList.slot_map_start()["1_download_start"]       = sigc::mem_fun(&Download::start);

  m_downloadList.slot_map_stop()["1_download_stop"]         = sigc::mem_fun(&Download::call<void, &torrent::Download::stop>);
  m_downloadList.slot_map_stop()["2_hash_resume_save"]      = sigc::mem_fun(&Download::call<void, &torrent::Download::hash_resume_save>);
  m_downloadList.slot_map_stop()["3_store_save"]            = sigc::mem_fun(m_downloadStore, &DownloadStore::save);

  m_downloadList.slot_map_finished()["1_download_done"]     = sigc::mem_fun(*this, &Manager::receive_download_done);
  m_downloadList.slot_map_finished()["2_receive_finished"]  = sigc::mem_fun(&Download::receive_finished);
}

void
Manager::cleanup() {
  // Need to disconnect log signals? Not really since we won't receive
  // any more.

  torrent::cleanup();
  CurlStack::global_cleanup();
}

void
Manager::shutdown(bool force) {
  if (!force)
    std::for_each(m_downloadList.begin(), m_downloadList.end(),
		  std::bind1st(std::mem_fun(&DownloadList::stop), &m_downloadList));
  else
    std::for_each(m_downloadList.begin(), m_downloadList.end(),
		  std::bind1st(std::mem_fun(&DownloadList::close), &m_downloadList));
}

Manager::DListItr
Manager::insert(std::istream* s) {
  try {
    return m_downloadList.insert(s);

  } catch (torrent::local_error& e) {
    m_logImportant.push_front(e.what());
    m_logComplete.push_front(e.what());

    return m_downloadList.end();
  }
}

Manager::DListItr
Manager::erase(DListItr itr) {
  if ((*itr)->get_download().is_active())
    throw std::logic_error("core::Manager::erase(...) called on an active download");

//   if (!(*itr)->get_download().is_open())
//     throw std::logic_error("core::Manager::erase(...) called on an closed download");

  return m_downloadList.erase(itr);
}  

void
Manager::start(Download* d) {
  try {
    d->get_bencode()["rtorrent"]["state"] = "started";

    if (d->get_download().is_active())
      return;

    if (!d->get_download().is_open())
      m_downloadList.open(d);

    if (d->get_download().is_hash_checked())
      m_downloadList.start(d);
    else
      // This can cause infinit loops?
      m_hashQueue.insert(d, sigc::bind(sigc::mem_fun(m_downloadList, &DownloadList::start), d));

  } catch (torrent::local_error& e) {
    m_logImportant.push_front(e.what());
    m_logComplete.push_front(e.what());
  }
}

void
Manager::stop(Download* d) {
  try {
    d->get_bencode()["rtorrent"]["state"] = "stopped";

    m_downloadList.stop(d);

  } catch (torrent::local_error& e) {
    m_logImportant.push_front(e.what());
    m_logComplete.push_front(e.what());
  }
}

void
Manager::check_hash(Download* d) {
  bool restart = d->get_download().is_active();

  try {
    prepare_hash_check(d);

    if (restart)
      m_hashQueue.insert(d, sigc::bind(sigc::mem_fun(m_downloadList, &DownloadList::start), d));
    else
      m_hashQueue.insert(d, sigc::slot0<void>());

  } catch (torrent::local_error& e) {
    m_logImportant.push_front(e.what());
    m_logComplete.push_front(e.what());
  }
}  

void
Manager::receive_download_done(Download* d) {
  if (m_checkHash) {
    // Start the hash checking, send completed to tracker after
    // finishing.
    prepare_hash_check(d);

    // TODO: Need to restart the torrent.
    m_hashQueue.insert(d, sigc::bind(sigc::mem_fun(*this, &Manager::receive_download_done_hash_checked), d));

  } else {
    receive_download_done_hash_checked(d);
  }
}

void
Manager::listen_open() {
  if (m_portFirst > m_portLast)
    throw std::runtime_error("Invalid port range for listening");

  if (m_portRandom) {
    int boundary = m_portFirst + random() % (m_portLast - m_portFirst + 1);

    if (!torrent::listen_open(boundary, m_portLast) &&
	!torrent::listen_open(m_portFirst, boundary))
      throw std::runtime_error("Could not open port for listening.");

  } else {
    if (!torrent::listen_open(m_portFirst, m_portLast))
      throw std::runtime_error("Could not open port for listening.");
  }
}

void
Manager::initialize_bencode(Download* d) {
  torrent::Bencode& bencode = d->get_bencode();

  if (!bencode.has_key("rtorrent") ||
      !bencode["rtorrent"].is_map())
    bencode.insert_key("rtorrent", torrent::Bencode(torrent::Bencode::TYPE_MAP));
    
  if (!bencode["rtorrent"].has_key("state") ||
      !bencode["rtorrent"]["state"].is_string())
    bencode["rtorrent"].insert_key("state", "started");
}

void
Manager::prepare_hash_check(Download* d) {
  m_downloadList.close(d);
  d->get_download().hash_resume_clear();
  m_downloadList.open(d);

  if (d->get_download().is_hash_checking() ||
      d->get_download().is_hash_checked())
    throw std::logic_error("Manager::check_hash(...) closed the torrent but is_hash_check{ing,ed}() == true");

  if (m_hashQueue.find(d) != m_hashQueue.end())
    throw std::logic_error("Manager::check_hash(...) closed the torrent but it was found in m_hashQueue");
}

void
Manager::receive_http_failed(std::string msg) {
  m_logImportant.push_front("Http download error: \"" + msg + "\"");
  m_logComplete.push_front("Http download error: \"" + msg + "\"");
}

void
Manager::receive_download_done_hash_checked(Download* d) {
  if (!d->get_download().is_active())
    m_downloadList.start(d);

  // Don't send if we did a hash check and found incompelete chunks.
  //if (d->is_done())
    d->get_download().tracker_send_completed();
}

}
