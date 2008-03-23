// rTorrent - BitTorrent client
// Copyright (C) 2005-2007, Jari Sundell
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

#include <cstdio>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <sys/select.h>
#include <rak/address_info.h>
#include <rak/error_number.h>
#include <rak/regex.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <sigc++/adaptors/bind.h>
#include <sigc++/adaptors/hide.h>
#include <torrent/object.h>
#include <torrent/connection_manager.h>
#include <torrent/error.h>
#include <torrent/exceptions.h>
#include <torrent/resume.h>
#include <torrent/tracker_list.h>

#include "rpc/parse_commands.h"
#include "utils/directory.h"
#include "utils/file_status_cache.h"

#include "globals.h"
#include "curl_get.h"
#include "control.h"
#include "download.h"
#include "download_factory.h"
#include "download_store.h"
#include "http_queue.h"
#include "manager.h"
#include "poll_manager_epoll.h"
#include "poll_manager_kqueue.h"
#include "poll_manager_select.h"
#include "view.h"

namespace core {

void
receive_tracker_dump(const std::string& url, const char* data, size_t size) {
  const std::string& filename = rpc::call_command_string("get_tracker_dump");

  if (filename.empty())
    return;

  std::fstream fstr(filename.c_str(), std::ios::out | std::ios::app);

  if (!fstr.is_open())
    return;

  fstr << "url: " << url << std::endl << "---" << std::endl;
  fstr.write(data, size);
  fstr << std::endl <<"---" << std::endl;
}

void
Manager::handshake_log(const sockaddr* sa, int msg, int err, const torrent::HashString* hash) {
  if (!rpc::call_command_value("get_handshake_log"))
    return;
  
  std::string peer;
  std::string download;

  const rak::socket_address* socketAddress = rak::socket_address::cast_from(sa);

  if (socketAddress->is_valid()) {
    char port[6];
    snprintf(port, sizeof(port), "%d", socketAddress->port());
    peer = socketAddress->address_str() + ":" + port;
  } else {
    peer = "(unknown)";
  }

//   torrent::Download d = torrent::download_find(hash);

//   if (d.is_valid())
//     download = ": " + d.name();
//   else
    download = "";

  switch (msg) {
  case torrent::ConnectionManager::handshake_incoming:
    m_logComplete.push_front("Incoming connection from " + peer + download);
    break;
  case torrent::ConnectionManager::handshake_outgoing:
    m_logComplete.push_front("Outgoing connection to " + peer + download);
    break;
  case torrent::ConnectionManager::handshake_outgoing_encrypted:
    m_logComplete.push_front("Outgoing encrypted connection to " + peer + download);
    break;
  case torrent::ConnectionManager::handshake_outgoing_proxy:
    m_logComplete.push_front("Outgoing proxy connection to " + peer + download);
    break;
  case torrent::ConnectionManager::handshake_success:
    m_logComplete.push_front("Successful handshake: " + peer + download);
    break;
  case torrent::ConnectionManager::handshake_dropped:
    m_logComplete.push_front("Dropped handshake: " + peer + " - " + torrent::strerror(err) + download);
    break;
  case torrent::ConnectionManager::handshake_failed:
    m_logComplete.push_front("Handshake failed: " + peer + " - " + torrent::strerror(err) + download);
    break;
  case torrent::ConnectionManager::handshake_retry_plaintext:
    m_logComplete.push_front("Trying again without encryption: " + peer + download);
    break;
  case torrent::ConnectionManager::handshake_retry_encrypted:
    m_logComplete.push_front("Trying again encrypted: " + peer + download);
    break;
  default:
    m_logComplete.push_front("Unknown handshake message for " + peer + download);
    break;
  }
}

void
Manager::push_log(const char* msg) {
  m_logImportant.push_front(msg);
  m_logComplete.push_front(msg);
}

Manager::Manager() :
  m_hashingView(NULL),

  m_pollManager(NULL) {

  m_downloadStore   = new DownloadStore();
  m_downloadList    = new DownloadList();
  m_fileStatusCache = new FileStatusCache();
  m_httpQueue       = new HttpQueue();
}

Manager::~Manager() {
  delete m_downloadList;

  delete m_downloadStore;
  delete m_httpQueue;
  delete m_fileStatusCache;
}

void
Manager::set_hashing_view(View* v) {
  if (v == NULL || m_hashingView != NULL)
    throw torrent::internal_error("Manager::set_hashing_view(...) received NULL or is already set.");

  m_hashingView = v;
  v->signal_changed().connect(sigc::mem_fun(this, &Manager::receive_hashing_changed));
}

void
Manager::initialize_first() {
  if ((m_pollManager = PollManagerEPoll::create(sysconf(_SC_OPEN_MAX))) != NULL)
    m_logImportant.push_front("Using 'epoll' based polling.");

  else if ((m_pollManager = PollManagerKQueue::create(sysconf(_SC_OPEN_MAX))) != NULL)
    m_logImportant.push_front("Using 'kqueue' based polling.");

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
  torrent::Http::set_factory(sigc::mem_fun(m_pollManager->get_http_stack(), &CurlStack::new_object));
  m_httpQueue->slot_factory(sigc::mem_fun(m_pollManager->get_http_stack(), &CurlStack::new_object));

  CurlStack::global_init();

  // Register slots to be called when a download is inserted/erased,
  // opened or closed.
  m_downloadList->slot_map_insert()["1_connect_logs"] = sigc::bind(&rpc::parse_command_d_single_std, "d.initialize_logs=");
  m_downloadList->slot_map_erase()["9_delete_tied"]   = sigc::bind(&rpc::parse_command_d_single_std, "d.delete_tied=");

  torrent::connection_manager()->set_signal_handshake_log(sigc::mem_fun(this, &Manager::handshake_log));
}

void
Manager::cleanup() {
  // Need to disconnect log signals? Not really since we won't receive
  // any more.

  m_downloadList->clear();

  // When we implement asynchronous DNS lookups, we need to cancel them
  // here before the torrent::* objects are deleted.

  torrent::cleanup();
  CurlStack::global_cleanup();

  delete m_pollManager;
}

void
Manager::shutdown(bool force) {
  if (!force)
    std::for_each(m_downloadList->begin(), m_downloadList->end(), std::bind1st(std::mem_fun(&DownloadList::pause_default), m_downloadList));
  else
    std::for_each(m_downloadList->begin(), m_downloadList->end(), std::bind1st(std::mem_fun(&DownloadList::close_quick), m_downloadList));
}

void
Manager::listen_open() {
  // This stuff really should be moved outside of manager, make it
  // part of the init script.
  if (!rpc::call_command_value("get_port_open"))
    return;

  int portFirst, portLast;
  torrent::Object portRange = rpc::call_command_void("get_port_range");

  if (portRange.is_string()) {
    if (std::sscanf(portRange.as_string().c_str(), "%i-%i", &portFirst, &portLast) != 2)
      throw torrent::input_error("Invalid port_range argument.");
    
//   } else if (portRange.is_list()) {

  } else {
    throw torrent::input_error("Invalid port_range argument.");
  }

  if (portFirst > portLast || portLast >= (1 << 16))
    throw torrent::input_error("Invalid port range.");

  if (rpc::call_command_value("get_port_random")) {
    int boundary = portFirst + random() % (portLast - portFirst + 1);

    if (torrent::connection_manager()->listen_open(boundary, portLast) ||
        torrent::connection_manager()->listen_open(portFirst, boundary))
      return;

  } else {
    if (torrent::connection_manager()->listen_open(portFirst, portLast))
      return;
  }

  throw torrent::input_error("Could not open/bind port for listening: " + std::string(rak::error_number::current().c_str()));
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

std::string
Manager::proxy_address() const {
  return rak::socket_address::cast_from(torrent::connection_manager()->proxy_address())->address_str();
}

void
Manager::set_proxy_address(const std::string& addr) {
  int port;
  rak::address_info* ai;

  char buf[addr.length() + 1];

  int err = std::sscanf(addr.c_str(), "%[^:]:%i", buf, &port);

  if (err <= 0)
    throw torrent::input_error("Could not parse proxy address.");

  if (err == 1)
    port = 80;

  if ((err = rak::address_info::get_address_info(buf, PF_INET, SOCK_STREAM, &ai)) != 0)
    throw torrent::input_error("Could not set proxy address: " + std::string(rak::address_info::strerror(err)) + ".");
  
  try {

    ai->address()->set_port(port);
    torrent::connection_manager()->set_proxy_address(ai->address()->c_sockaddr());
    
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
Manager::try_create_download(const std::string& uri, int flags, const command_list_type& commands) {
  // If the path was attempted loaded before, skip it.
  if (!(flags & create_raw_data) &&
      !is_network_uri(uri) &&
      !file_status_cache()->insert(uri, 0))
    return;

  // Adding download.
  DownloadFactory* f = new DownloadFactory(this);

  f->variables()["tied_to_file"] = (int64_t)(bool)(flags & create_tied);
  f->commands().insert(f->commands().end(), commands.begin(), commands.end());

  f->set_start(flags & create_start);
  f->set_print_log(!(flags & create_quiet));
  f->slot_finished(sigc::bind(sigc::ptr_fun(&rak::call_delete_func<core::DownloadFactory>), f));

  if (flags & create_raw_data)
    f->load_raw_data(uri);
  else
    f->load(uri);

  f->commit();
}

utils::Directory
path_expand_transform(std::string path, const utils::directory_entry& entry) {
  return path + entry.d_name;
}

// Move this somewhere better.
void
path_expand(std::vector<std::string>* paths, const std::string& pattern) {
  std::vector<utils::Directory> currentCache;
  std::vector<utils::Directory> nextCache;

  rak::split_iterator_t<std::string> first = rak::split_iterator(pattern, '/');
  rak::split_iterator_t<std::string> last  = rak::split_iterator(pattern);
    
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
      itr->update((r.pattern()[0] != '.') ? utils::Directory::update_hide_dot : 0);
      itr->erase(std::remove_if(itr->begin(), itr->end(), rak::on(rak::mem_ref(&utils::directory_entry::d_name), std::not1(r))), itr->end());

      std::transform(itr->begin(), itr->end(), std::back_inserter(nextCache), rak::bind1st(std::ptr_fun(&path_expand_transform), itr->path() + "/"));
    }

    currentCache.clear();
    currentCache.swap(nextCache);
  }

  std::transform(currentCache.begin(), currentCache.end(), std::back_inserter(*paths), std::mem_fun_ref(&utils::Directory::path));
}

bool
manager_equal_tied(const std::string& path, Download* download) {
  return path == rpc::call_command_string("d.get_tied_to_file", rpc::make_target(download));
}

void
Manager::try_create_download_expand(const std::string& uri, int flags, command_list_type commands) {
  if (flags & create_raw_data) {
    try_create_download(uri, flags, commands);
    return;
  }

  std::vector<std::string> paths;
  paths.reserve(256);

  path_expand(&paths, uri);

  if (!paths.empty())
    for (std::vector<std::string>::iterator itr = paths.begin(); itr != paths.end(); ++itr)
      try_create_download(*itr, flags, commands);

  else
    try_create_download(uri, flags, commands);
}

// DownloadList's hashing related functions don't actually start the
// hashing, it only reacts to events. This functions checks the
// hashing view and starts hashing if nessesary.
void
Manager::receive_hashing_changed() {
  bool foundHashing = std::find_if(m_hashingView->begin_visible(), m_hashingView->end_visible(),
                                   std::mem_fun(&Download::is_hash_checking)) != m_hashingView->end_visible();
  
  // Try quick hashing all those with hashing == initial, set them to
  // something else when failed.
  for (View::iterator itr = m_hashingView->begin_visible(), last = m_hashingView->end_visible(); itr != last; ++itr) {
    if ((*itr)->is_hash_checked())
      throw torrent::internal_error("core::Manager::receive_hashing_changed() (*itr)->is_hash_checked().");
  
    if ((*itr)->is_hash_checking() || (*itr)->is_hash_failed())
      continue;

    bool tryQuick =
      rpc::call_command_value("d.get_hashing", rpc::make_target(*itr)) == Download::variable_hashing_initial &&
      (*itr)->download()->file_list()->bitfield()->empty();

    if (!tryQuick && foundHashing)
      continue;

    try {
      m_downloadList->open_throw(*itr);

      // Since the bitfield is allocated on loading of resume load or
      // hash start, and unallocated on close, we know that if it it
      // not empty then we have already loaded any existing resume
      // data.
      if ((*itr)->download()->file_list()->bitfield()->empty())
        torrent::resume_load_progress(*(*itr)->download(), (*itr)->download()->bencode()->get_key("libtorrent_resume"));

      if (tryQuick) {
        if ((*itr)->download()->hash_check(true))
          continue;

        (*itr)->download()->hash_stop();

        if (foundHashing) {
          rpc::call_command_set_value("d.set_hashing", Download::variable_hashing_rehash, rpc::make_target(*itr));
          continue;
        }
      }

      (*itr)->download()->hash_check(false);
      foundHashing = true;

    } catch (torrent::local_error& e) {
      if (tryQuick) {
        // Make sure we don't repeat the quick hashing.
        rpc::call_command_set_value("d.set_hashing", Download::variable_hashing_rehash, rpc::make_target(*itr));

      } else {
        (*itr)->set_hash_failed(true);
        push_log(e.what());
      }
    }
  }
}

}
