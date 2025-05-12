#include "config.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/select.h>
#include <rak/address_info.h>
#include <rak/error_number.h>
#include <rak/regex.h>
#include <rak/path.h>
#include <rak/string_manip.h>
#include <torrent/utils/resume.h>
#include <torrent/object.h>
#include <torrent/connection_manager.h>
#include <torrent/error.h>
#include <torrent/exceptions.h>
#include <torrent/object_stream.h>
#include <torrent/tracker_list.h>
#include <torrent/throttle.h>
#include <torrent/utils/log.h>

#include "rpc/parse_commands.h"
#include "utils/directory.h"
#include "utils/base64.h"
#include "utils/file_status_cache.h"

#include "globals.h"
#include "curl_get.h"
#include "curl_stack.h"
#include "control.h"
#include "download.h"
#include "download_factory.h"
#include "download_store.h"
#include "http_queue.h"
#include "manager.h"
#include "view.h"

namespace core {

const int Manager::create_start;
const int Manager::create_tied;
const int Manager::create_quiet;
const int Manager::create_raw_data;

void
Manager::push_log(const char* msg) {
  m_log_important->lock_and_push_log(msg, strlen(msg), 0);
  m_log_complete->lock_and_push_log(msg, strlen(msg), 0);
}

Manager::Manager() :
    m_hashingView(nullptr),
    m_log_important(torrent::log_open_log_buffer("important")),
    m_log_complete(torrent::log_open_log_buffer("complete")) {

  m_download_store    = std::make_unique<DownloadStore>();
  m_download_list     = std::make_unique<DownloadList>();
  m_file_status_cache = std::make_unique<FileStatusCache>();
  m_http_queue        = std::make_unique<HttpQueue>();
  m_http_stack        = std::make_unique<CurlStack>();

  torrent::Throttle* unthrottled = torrent::Throttle::create_throttle();
  unthrottled->set_max_rate(0);
  m_throttles["NULL"] = std::make_pair(unthrottled, unthrottled);
}

Manager::~Manager() {
  torrent::Throttle::destroy_throttle(m_throttles["NULL"].first);
}

void
Manager::set_hashing_view(View* v) {
  if (v == nullptr || m_hashingView != nullptr)
    throw torrent::internal_error("Manager::set_hashing_view(...) received nullptr or is already set.");

  m_hashingView = v;
  m_hashingView->signal_changed().push_back(std::bind(&Manager::receive_hashing_changed, this));
}

torrent::ThrottlePair
Manager::get_throttle(const std::string& name) {
  ThrottleMap::const_iterator itr = m_throttles.find(name);
  torrent::ThrottlePair throttles = (itr == m_throttles.end() ? torrent::ThrottlePair(nullptr, nullptr) : itr->second);

  if (throttles.first == nullptr)
    throttles.first = torrent::up_throttle_global();

  if (throttles.second == nullptr)
    throttles.second = torrent::down_throttle_global();

  return throttles;
}

void
Manager::set_address_throttle(uint32_t begin, uint32_t end, torrent::ThrottlePair throttles) {
  m_addressThrottles.set_merge(begin, end, throttles);
  torrent::connection_manager()->address_throttle() = std::bind(&core::Manager::get_address_throttle, control->core(), std::placeholders::_1);
}

torrent::ThrottlePair
Manager::get_address_throttle(const sockaddr* addr) {
  return m_addressThrottles.get(rak::socket_address::cast_from(addr)->sa_inet()->address_h(), torrent::ThrottlePair(nullptr, nullptr));
}

int64_t
Manager::retrieve_throttle_value(const torrent::Object::string_type& name, bool rate, bool up) {
  ThrottleMap::iterator itr = throttles().find(name);

  if (itr == throttles().end()) {
    return (int64_t)-1;
  } else {
    torrent::Throttle* throttle = up ? itr->second.first : itr->second.second;

    // check whether the actual up/down throttle exist (one of the pair can be missing)
    if (throttle == nullptr)
      return (int64_t)-1;

    int64_t throttle_max = (int64_t)throttle->max_rate();

    if (rate) {

      if (throttle_max > 0)
        return (int64_t)throttle->rate()->rate();
      else
        return (int64_t)-1;

    } else {
      return throttle_max;
    }

  }
}

// Most of this should be possible to move out.
void
Manager::initialize_second() {
  torrent::Http::slot_factory() = std::bind(&CurlStack::new_object, m_http_stack.get());
  m_http_queue->set_slot_factory(std::bind(&CurlStack::new_object, m_http_stack.get()));

  CurlStack::global_init();
}

void
Manager::cleanup() {
  m_http_stack->shutdown();

  // Need to disconnect log signals? Not really since we won't receive
  // any more.

  m_download_list->clear();

  // When we implement asynchronous DNS lookups, we need to cancel them
  // here before the torrent::* objects are deleted.

  torrent::cleanup();

  m_http_stack.reset();
  CurlStack::global_cleanup();
}

void
Manager::shutdown(bool force) {
  if (!force)
    std::for_each(m_download_list->begin(), m_download_list->end(), [this](Download* d) { m_download_list->pause_default(d); });
  else
    std::for_each(m_download_list->begin(), m_download_list->end(), [this](Download* d) { m_download_list->close_quick(d); });
}

void
Manager::listen_open() {
  // This stuff really should be moved outside of manager, make it
  // part of the init script.
  if (!rpc::call_command_value("network.port_open"))
    return;

  int portFirst, portLast;
  torrent::Object portRange = rpc::call_command("network.port_range");

  if (portRange.is_string()) {
    if (std::sscanf(portRange.as_string().c_str(), "%i-%i", &portFirst, &portLast) != 2)
      throw torrent::input_error("Invalid port_range argument.");

//   } else if (portRange.is_list()) {

  } else {
    throw torrent::input_error("Invalid port_range argument.");
  }

  if (portFirst > portLast || portLast >= (1 << 16))
    throw torrent::input_error("Invalid port range.");

  if (rpc::call_command_value("network.port_random")) {
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

  if ((err = rak::address_info::get_address_info(addr.c_str(), PF_INET, SOCK_STREAM, &ai)) != 0 &&
      (err = rak::address_info::get_address_info(addr.c_str(), PF_INET6, SOCK_STREAM, &ai)) != 0)
    throw torrent::input_error("Could not set bind address: " + std::string(rak::address_info::strerror(err)) + ".");

  try {

    if (torrent::connection_manager()->listen_port() != 0) {
      torrent::connection_manager()->listen_close();
      torrent::connection_manager()->set_bind_address(ai->address()->c_sockaddr());
      listen_open();

    } else {
      torrent::connection_manager()->set_bind_address(ai->address()->c_sockaddr());
    }

    m_http_stack->set_bind_address(!ai->address()->is_address_any() ? ai->address()->address_str() : std::string());

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

  if ((err = rak::address_info::get_address_info(addr.c_str(), PF_INET, SOCK_STREAM, &ai)) != 0 &&
      (err = rak::address_info::get_address_info(addr.c_str(), PF_INET6, SOCK_STREAM, &ai)) != 0)
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

  std::string buf(addr.length() + 1, '\0');

  int err = std::sscanf(addr.c_str(), "%[^:]:%i", buf.data(), &port);

  if (err <= 0)
    throw torrent::input_error("Could not parse proxy address.");

  if (err == 1)
    port = 80;

  if ((err = rak::address_info::get_address_info(buf.data(), PF_INET, SOCK_STREAM, &ai)) != 0)
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
  push_log_std("Http download error: \"" + msg + "\"");
}

bool
is_data_uri(const std::string& uri) {
  return std::strncmp(uri.c_str(), "data:", 5) == 0;
}

std::string
decode_data_uri(const std::string& uri) {
  const auto start = uri.find("base64,", 5) + 7;
  if (start == std::string::npos)
    throw torrent::input_error("Invalid data uri: not base64 encoded.");
  if (start >= uri.size())
    throw torrent::input_error("Empty base64.");
  return utils::decode_base64(uri.substr(start));
}

void
Manager::try_create_download(const std::string& uri, int flags, const command_list_type& commands) {
  // If the path was attempted loaded before, skip it.
  if ((flags & create_tied) &&
      !(flags & create_raw_data) &&
      !is_network_uri(uri) &&
      !is_magnet_uri(uri) &&
      !is_data_uri(uri) &&
      !file_status_cache()->insert(uri, 0))
    return;

  // Adding download.
  DownloadFactory* f = new DownloadFactory(this);

  f->variables()["tied_to_file"] = (int64_t)(bool)(flags & create_tied);
  f->commands().insert(f->commands().end(), commands.begin(), commands.end());

  f->set_start(flags & create_start);
  f->set_print_log(!(flags & create_quiet));
  f->slot_finished([f]() { delete f; });

  if (is_data_uri(uri)) {
    // Allow the use of data URIs, primarily for JSON-RPC which
    // doesn't have a defined mechanism for binary data
    f->load_raw_data(decode_data_uri(uri));
    f->variables()["tied_to_file"] = (int64_t)false;
  } else if (flags & create_raw_data) {
    f->load_raw_data(uri);
  } else {
    f->load(uri);
  }

  f->commit();
}

void
Manager::try_create_download_from_meta_download(torrent::Object* bencode, const std::string& metafile) {
  DownloadFactory* f = new DownloadFactory(this);

  f->variables()["tied_to_file"] = (int64_t)true;
  f->variables()["tied_file"] = metafile;

  torrent::Object& meta = bencode->get_key("rtorrent_meta_download");
  torrent::Object::list_type& commands = meta.get_key_list("commands");
  for (torrent::Object::list_type::const_iterator itr = commands.begin(); itr != commands.end(); ++itr)
    f->commands().insert(f->commands().end(), itr->as_string());

  f->set_start(meta.get_key_value("start"));
  f->set_print_log(meta.get_key_value("print_log"));
  f->slot_finished([f]() { delete f; });

  // Bit of a waste to create the bencode repesentation here
  // only to have the DownloadFactory decode it.
  std::stringstream s;
  s.imbue(std::locale::classic());
  s << *bencode;
  f->load_raw_data(s.str());
  f->commit();
}

utils::Directory
path_expand_transform(std::string path, const utils::directory_entry& entry) {
  return path + entry.s_name;
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
      itr->erase(std::remove_if(itr->begin(), itr->end(), [r](const utils::directory_entry& entry) { return !r(entry.s_name); }), itr->end());

      std::transform(itr->begin(), itr->end(), std::back_inserter(nextCache), [itr](const utils::directory_entry& entry) {
          return path_expand_transform(itr->path() + (itr->path() == "/" ? "" : "/"), entry);
        });
    }

    currentCache.clear();
    currentCache.swap(nextCache);
  }

  std::transform(currentCache.begin(), currentCache.end(), std::back_inserter(*paths), std::mem_fn(&utils::Directory::path));
}

bool
manager_equal_tied(const std::string& path, Download* download) {
  return path == rpc::call_command_string("d.tied_to_file", rpc::make_target(download));
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
                                   std::mem_fn(&Download::is_hash_checking)) != m_hashingView->end_visible();

  // Try quick hashing all those with hashing == initial, set them to
  // something else when failed.
  for (View::iterator itr = m_hashingView->begin_visible(), last = m_hashingView->end_visible(); itr != last; ++itr) {
    if ((*itr)->is_hash_checked())
      throw torrent::internal_error("core::Manager::receive_hashing_changed() (*itr)->is_hash_checked().");

    if ((*itr)->is_hash_checking() || (*itr)->is_hash_failed())
      continue;

    bool tryQuick =
      rpc::call_command_value("d.hashing", rpc::make_target(*itr)) == Download::variable_hashing_initial &&
      (*itr)->download()->file_list()->bitfield()->empty();

    if (!tryQuick && foundHashing)
      continue;

    try {
      m_download_list->open_throw(*itr);

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
          rpc::call_command_set_value("d.hashing.set", Download::variable_hashing_rehash, rpc::make_target(*itr));
          continue;
        }
      }

      (*itr)->download()->hash_check(false);
      foundHashing = true;

    } catch (torrent::local_error& e) {
      if (tryQuick) {
        // Make sure we don't repeat the quick hashing.
        rpc::call_command_set_value("d.hashing.set", Download::variable_hashing_rehash, rpc::make_target(*itr));

      } else {
        (*itr)->set_hash_failed(true);
        lt_log_print(torrent::LOG_TORRENT_ERROR, "Hashing failed: %s", e.what());
      }
    }
  }
}

}
