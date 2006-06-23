// rTorrent - BitTorrent client
// Copyright (C) 2005-2006, Jari Sundell
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
#include <unistd.h>
#include <sys/select.h>
#include <rak/address_info.h>
#include <rak/error_number.h>
#include <rak/regex.h>
#include <rak/string_manip.h>
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <torrent/object.h>
#include <torrent/connection_manager.h>
#include <torrent/exceptions.h>
#include <torrent/tracker_list.h>

#include "utils/variable_map.h"

#include "globals.h"
#include "curl_get.h"
#include "download.h"
#include "download_factory.h"
#include "download_store.h"
#include "http_queue.h"
#include "manager.h"
#include "poll_manager_epoll.h"
#include "poll_manager_select.h"
#include "view.h"

namespace core {

static void
connect_signal_network_log(Download* d, torrent::Download::slot_string_type s) {
  d->download()->signal_network_log(s);
}

static void
connect_signal_storage_log(Download* d, torrent::Download::slot_string_type s) {
  d->download()->signal_storage_error(s);
}

// Need a proper logging class for this.
static void
connect_signal_tracker_dump(Download* d, torrent::Download::slot_dump_type s) {
  if (!control->variable()->get_string("tracker_dump").empty())
    d->download()->signal_tracker_dump(s);
}

static void
receive_tracker_dump(const std::string& url, const char* data, size_t size) {
  const std::string& filename = control->variable()->get_string("tracker_dump");

  if (filename.empty())
    return;

  std::fstream fstr(filename.c_str(), std::ios::out | std::ios::app);

  if (!fstr.is_open())
    return;

  fstr << "url: " << url << std::endl << "---" << std::endl;
  fstr.write(data, size);
  fstr << std::endl <<"---" << std::endl;
}

// Hmm... find some better place for all this.
static void
delete_tied(Download* d) {
  const std::string tie = d->variable()->get_string("tied_to_file");

  // This should be configurable, need to wait for the variable
  // thingie to be implemented.
  if (tie.empty())
    return;

  if (::unlink(tie.c_str()) == -1)
    control->core()->push_log("Could not unlink tied file: " + std::string(rak::error_number::current().c_str()));
}

Manager::Manager() :
  m_hashingView(NULL),

  m_pollManager(NULL),
  m_portFirst(6890),
  m_portLast(6999) {

  m_downloadStore = new DownloadStore();
  m_downloadList = new DownloadList();

  m_httpQueue = new HttpQueue();
}

Manager::~Manager() {
  delete m_downloadList;

  delete m_downloadStore;
  delete m_httpQueue;
}

void
Manager::set_hashing_view(View* v) {
  if (v == NULL || m_hashingView != NULL)
    throw torrent::client_error("Manager::set_hashing_view(...) received NULL or is already set.");

  m_hashingView = v;
  v->signal_changed().connect(sigc::mem_fun(this, &Manager::receive_hashing_changed));
}

void
Manager::initialize_first() {
  if ((m_pollManager = PollManagerEPoll::create(sysconf(_SC_OPEN_MAX))) != NULL)
    m_logImportant.push_front("Using 'epoll' based polling.");
  else if ((m_pollManager = PollManagerSelect::create(sysconf(_SC_OPEN_MAX))) != NULL)
    m_logImportant.push_front("Using 'select' based polling.");
  else
    throw std::runtime_error("Could not create any PollManager.");

  // Need to initialize this before parseing options.
  torrent::initialize(m_pollManager->get_torrent_poll());
}

// Most of this should be possible to move out.
void
Manager::initialize_second() {
  torrent::Http::set_factory(m_pollManager->get_http_stack()->get_http_factory());
  m_httpQueue->slot_factory(m_pollManager->get_http_stack()->get_http_factory());

  CurlStack::global_init();

  // Register slots to be called when a download is inserted/erased,
  // opened or closed.
  m_downloadList->slot_map_insert()["1_connect_network_log"]  = sigc::bind(sigc::ptr_fun(&connect_signal_network_log), sigc::mem_fun(m_logComplete, &Log::push_front));
  m_downloadList->slot_map_insert()["1_connect_storage_log"]  = sigc::bind(sigc::ptr_fun(&connect_signal_storage_log), sigc::mem_fun(m_logComplete, &Log::push_front));
  m_downloadList->slot_map_insert()["1_connect_tracker_dump"] = sigc::bind(sigc::ptr_fun(&connect_signal_tracker_dump), sigc::ptr_fun(&receive_tracker_dump));

  m_downloadList->slot_map_erase()["1_delete_tied"] = sigc::ptr_fun(&delete_tied);
}

void
Manager::cleanup() {
  // Need to disconnect log signals? Not really since we won't receive
  // any more.

  m_downloadList->clear();

  torrent::cleanup();
  CurlStack::global_cleanup();

  delete m_pollManager;
}

void
Manager::shutdown(bool force) {
  // This doesn't trigger a compiler error on gcc-3.4.5 for some reason.
//   if (!force)
//     std::for_each(m_downloadList->begin(), m_downloadList->end(), std::bind1st(std::mem_fun(&DownloadList::pause), &m_downloadList));
//   else
//     std::for_each(m_downloadList->begin(), m_downloadList->end(), std::bind1st(std::mem_fun(&DownloadList::close), &m_downloadList));

  if (!force)
    std::for_each(m_downloadList->begin(), m_downloadList->end(), std::bind1st(std::mem_fun(&DownloadList::pause), m_downloadList));
  else
    std::for_each(m_downloadList->begin(), m_downloadList->end(), std::bind1st(std::mem_fun(&DownloadList::close), m_downloadList));
}

void
Manager::listen_open() {
  if (!control->variable()->get_value("port_open"))
    return;

  if (m_portFirst > m_portLast)
    throw torrent::input_error("Invalid port range for listening");

  if (control->variable()->get_value("port_random")) {
    int boundary = m_portFirst + random() % (m_portLast - m_portFirst + 1);

    if (torrent::connection_manager()->listen_open(boundary, m_portLast) ||
	torrent::connection_manager()->listen_open(m_portFirst, boundary))
      return;

  } else {
    if (torrent::connection_manager()->listen_open(m_portFirst, m_portLast))
      return;
  }

  throw torrent::input_error("Could not open/bind a port for listening: " + std::string(rak::error_number::current().c_str()));
}

std::string
Manager::bind_address() const {
  return rak::socket_address::cast_from(torrent::connection_manager()->bind_address())->address_str();
}

void
Manager::set_bind_address(const std::string& addr) {
  int err;
  rak::address_info* ai;

  if ((err = rak::address_info::get_address_info(addr.c_str(), PF_INET, SOCK_STREAM, &ai)) != 0)
    throw torrent::input_error("Could not set bind address: " + std::string(rak::address_info::strerror(err)) + ".");
  
  try {

    if (torrent::connection_manager()->listen_port() != 0) {
      torrent::connection_manager()->listen_close();
      torrent::connection_manager()->set_bind_address(ai->address()->c_sockaddr());
      listen_open();

    } else {
      torrent::connection_manager()->set_bind_address(ai->address()->c_sockaddr());
    }

    m_pollManager->get_http_stack()->set_bind_address(!ai->address()->is_address_any() ? ai->address()->address_str() : std::string());

    rak::address_info::free_address_info(ai);

  } catch (torrent::input_error& e) {
    rak::address_info::free_address_info(ai);
    throw e;
  }
}

std::string
Manager::local_address() const {
  return rak::socket_address::cast_from(torrent::connection_manager()->local_address())->address_str();
}

void
Manager::set_local_address(const std::string& addr) {
  int err;
  rak::address_info* ai;

  if ((err = rak::address_info::get_address_info(addr.c_str(), PF_INET, SOCK_STREAM, &ai)) != 0)
    throw torrent::input_error("Could not set local address: " + std::string(rak::address_info::strerror(err)) + ".");
  
  try {

    torrent::connection_manager()->set_local_address(ai->address()->c_sockaddr());
    rak::address_info::free_address_info(ai);

  } catch (torrent::input_error& e) {
    rak::address_info::free_address_info(ai);
    throw e;
  }
}

void
Manager::receive_http_failed(std::string msg) {
  m_logImportant.push_front("Http download error: \"" + msg + "\"");
  m_logComplete.push_front("Http download error: \"" + msg + "\"");
}

void
Manager::try_create_download(const std::string& uri, bool start, bool printLog, bool tied) {
  // Adding download.
  DownloadFactory* f = new DownloadFactory(uri, this);

  f->variable()->set("tied_to_file", tied ? "yes" : "no");

  f->set_start(start);
  f->set_print_log(printLog);
  f->slot_finished(sigc::bind(sigc::ptr_fun(&rak::call_delete_func<core::DownloadFactory>), f));
  f->load();
  f->commit();
}

// Move this somewhere better.
void
path_expand(std::vector<std::string>* paths, const std::string& pattern) {
  std::vector<utils::Directory> currentCache;
  std::vector<utils::Directory> nextCache;

  rak::split_iterator_t<std::string> first = rak::split_iterator(pattern, '/');
  rak::split_iterator_t<std::string> last = rak::split_iterator(pattern);
    
  if (first == last)
    return;

  // Check for initial '/' that indicates the root.
  if ((*first).empty()) {
    currentCache.push_back(utils::Directory("/"));
    ++first;
  } else if (rak::trim(*first) == "~") {
    currentCache.push_back(utils::Directory("~"));
    ++first;
  } else {
    currentCache.push_back(utils::Directory("."));
  }

  // Might be an idea to use depth-first search instead.

  for (; first != last; ++first) {
    rak::regex r(*first);

    if (r.pattern().empty())
      continue;

    // Special case for ".."?

    for (std::vector<utils::Directory>::iterator itr = currentCache.begin(); itr != currentCache.end(); ++itr) {
      // Only include filenames starting with '.' if the pattern
      // starts with the same.
      itr->update(r.pattern()[0] != '.');
      itr->erase(std::remove_if(itr->begin(), itr->end(), std::not1(r)), itr->end());

      std::transform(itr->begin(), itr->end(), std::back_inserter(nextCache), std::bind1st(std::plus<std::string>(), itr->get_path() + "/"));
    }

    currentCache.clear();
    currentCache.swap(nextCache);
  }

  std::transform(currentCache.begin(), currentCache.end(), std::back_inserter(*paths), std::mem_fun_ref(&utils::Directory::get_path));
}

void
Manager::try_create_download_expand(const std::string& uri, bool start, bool printLog, bool tied) {
  std::vector<std::string> paths;
  paths.reserve(256);

  path_expand(&paths, uri);

  if (tied)
    for (std::vector<std::string>::iterator itr = paths.begin(); itr != paths.end(); )
      if (std::find_if(m_downloadList->begin(), m_downloadList->end(), rak::equal(*itr, rak::bind2nd(std::mem_fun(&Download::variable_string), "tied_to_file")))
          != m_downloadList->end())
        itr = paths.erase(itr);
      else
        itr++;

  if (!paths.empty())
    for (std::vector<std::string>::iterator itr = paths.begin(); itr != paths.end(); ++itr)
      try_create_download(*itr, start, printLog, tied);

  else
    try_create_download(uri, start, printLog, tied);
}

// DownloadList's hashing related functions don't actually start the
// hashing, it only reacts to events. This functions checks the
// hashing view and starts hashing if nessesary.
void
Manager::receive_hashing_changed() {
  if (m_hashingView->empty_visible() ||
      std::find_if(m_hashingView->begin_visible(), m_hashingView->end_visible(), std::mem_fun(&Download::is_hash_checking)) != m_hashingView->end_visible())
    return;

  for (View::iterator itr = m_hashingView->begin_visible(), last = m_hashingView->end_visible(); itr != last; ++itr) {

    try {

      if ((*itr)->is_hash_checked() || (*itr)->is_hash_checking())
        throw torrent::client_error("core::Manager::receive_hashing_changed() hash already checked or checking.");
  
      if ((*itr)->is_hash_failed())
        continue;

      m_downloadList->open_throw(*itr);
      (*itr)->download()->hash_check();

      return;

    } catch (torrent::local_error& e) {
      (*itr)->set_hash_failed(true);
      push_log(e.what());
    }
  }
}

}
