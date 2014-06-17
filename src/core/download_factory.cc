// rTorrent - BitTorrent client
// Copyright (C) 2005-2011, Jari Sundell
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

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <rak/path.h>
#include lt_tr1_functional
#include <torrent/utils/log.h>
#include <torrent/utils/resume.h>
#include <torrent/object.h>
#include <torrent/object_stream.h>
#include <torrent/exceptions.h>
#include <torrent/rate.h>
#include <torrent/data/file_utils.h>

#include "rpc/parse_commands.h"

#include "curl_get.h"
#include "control.h"
#include "http_queue.h"
#include "globals.h"
#include "manager.h"

#include "download.h"
#include "download_factory.h"
#include "download_store.h"

namespace core {

bool
is_network_uri(const std::string& uri) {
  return
    std::strncmp(uri.c_str(), "http://", 7) == 0 ||
    std::strncmp(uri.c_str(), "https://", 8) == 0 ||
    std::strncmp(uri.c_str(), "ftp://", 6) == 0;
}

static bool
download_factory_add_stream(torrent::Object* root, const char* key, const char* filename) {
  std::fstream stream(filename, std::ios::in | std::ios::binary);
    
  if (!stream.is_open())
    return false;

  torrent::Object obj;
  stream >> obj;

  if (!stream.good())
    return false;

  root->insert_key_move(key, obj);
  return true;
}

bool
is_magnet_uri(const std::string& uri) {
  return
    std::strncmp(uri.c_str(), "magnet:?", 8) == 0;
}

DownloadFactory::DownloadFactory(Manager* m) :
  m_manager(m),
  m_stream(NULL),
  m_object(NULL),
  m_commited(false),
  m_loaded(false),

  m_session(false),
  m_start(false),
  m_printLog(true),
  m_isFile(false) {

  m_taskLoad.slot() = std::bind(&DownloadFactory::receive_load, this);
  m_taskCommit.slot() = std::bind(&DownloadFactory::receive_commit, this);

  // m_variables["connection_leech"] = rpc::call_command("protocol.connection.leech");
  // m_variables["connection_seed"]  = rpc::call_command("protocol.connection.seed");
  m_variables["connection_leech"] = std::string();
  m_variables["connection_seed"]  = std::string();
  m_variables["directory"]        = rpc::call_command("directory.default");
  m_variables["tied_to_file"]     = torrent::Object((int64_t)false);
}

DownloadFactory::~DownloadFactory() {
  priority_queue_erase(&taskScheduler, &m_taskLoad);
  priority_queue_erase(&taskScheduler, &m_taskCommit);

  delete m_stream;
  delete m_object;
  m_stream = NULL;
}

void
DownloadFactory::load(const std::string& uri) {
  m_uri = uri;
  priority_queue_insert(&taskScheduler, &m_taskLoad, cachedTime);
}

// This function must be called before DownloadFactory::commit().
void
DownloadFactory::load_raw_data(const std::string& input) {
  if (m_stream)
    throw torrent::internal_error("DownloadFactory::load*() called on an object with m_stream != NULL");

  m_stream = new std::stringstream(input);
  m_loaded = true;
}

void
DownloadFactory::commit() {
  priority_queue_insert(&taskScheduler, &m_taskCommit, cachedTime);
}

void
DownloadFactory::receive_load() {
  if (m_stream)
    throw torrent::internal_error("DownloadFactory::load*() called on an object with m_stream != NULL");

  if (is_network_uri(m_uri)) {
    // Http handling here.
    m_stream = new std::stringstream;
    HttpQueue::iterator itr = m_manager->http_queue()->insert(m_uri, m_stream);

    (*itr)->signal_done().push_front(std::bind(&DownloadFactory::receive_loaded, this));
    (*itr)->signal_failed().push_front(std::bind(&DownloadFactory::receive_failed, this, std::placeholders::_1));

    m_variables["tied_to_file"] = (int64_t)false;

  } else if (is_magnet_uri(m_uri)) {
    // DEBUG: Use m_object.
    m_stream = new std::stringstream();
    *m_stream << "d10:magnet-uri" << m_uri.length() << ":" << m_uri << "e";

    m_variables["tied_to_file"] = (int64_t)false;
    receive_loaded();

  } else {
    std::fstream stream(rak::path_expand(m_uri).c_str(), std::ios::in | std::ios::binary);

    if (!stream.is_open())
      return receive_failed("Could not open file");

    m_object = new torrent::Object;
    stream >> *m_object;

    if (!stream.good())
      return receive_failed("Reading torrent file failed");

    m_isFile = true;

    receive_loaded();
  }
}

void
DownloadFactory::receive_loaded() {
  m_loaded = true;

  if (m_commited)
    receive_success();
}

void
DownloadFactory::receive_commit() {
  m_commited = true;

  if (m_loaded)
    receive_success();
}

void
DownloadFactory::receive_success() {
  Download* download = m_stream != NULL ?
    m_manager->download_list()->create(m_stream, m_printLog) :
    m_manager->download_list()->create(m_object, m_printLog);

  m_object = NULL;

  if (download == NULL) {
    // core::Manager should already have added the error message to
    // the log.
    m_slot_finished();
    return;
  }

  torrent::Object* root = download->bencode();

  if (download->download()->info()->is_meta_download()) {
    torrent::Object& meta = root->insert_key("rtorrent_meta_download", torrent::Object::create_map());
    meta.insert_key("start", m_start);
    meta.insert_key("print_log", m_printLog);
 
    torrent::Object::list_type& commands = meta.insert_key("commands", torrent::Object::create_list()).as_list();

    for (command_list_type::iterator itr = m_commands.begin(); itr != m_commands.end(); ++itr)
      commands.push_back(*itr);
  }

  if (m_session) {
    download_factory_add_stream(root, "rtorrent", (rak::path_expand(m_uri) + ".rtorrent").c_str());
    download_factory_add_stream(root, "libtorrent_resume", (rak::path_expand(m_uri) + ".libtorrent_resume").c_str());
    
  } else {
    // We only allow session torrents to keep their
    // 'rtorrent/libtorrent' sections. The "fast_resume" section
    // should be safe to keep.
    root->erase_key("rtorrent");
  }

  torrent::Object* rtorrent = &root->insert_preserve_copy("rtorrent", torrent::Object::create_map()).first->second;
  torrent::Object& resumeObject = root->insert_preserve_copy("libtorrent_resume", torrent::Object::create_map()).first->second;

  initialize_rtorrent(download, rtorrent);

  if (!rtorrent->has_key_string("custom1")) rtorrent->insert_key("custom1", std::string());
  if (!rtorrent->has_key_string("custom2")) rtorrent->insert_key("custom2", std::string());
  if (!rtorrent->has_key_string("custom3")) rtorrent->insert_key("custom3", std::string());
  if (!rtorrent->has_key_string("custom4")) rtorrent->insert_key("custom4", std::string());
  if (!rtorrent->has_key_string("custom5")) rtorrent->insert_key("custom5", std::string());

  rpc::call_command("d.uploads_min.set",      rpc::call_command("throttle.min_uploads"), rpc::make_target(download));
  rpc::call_command("d.uploads_max.set",      rpc::call_command("throttle.max_uploads"), rpc::make_target(download));
  rpc::call_command("d.downloads_min.set",    rpc::call_command("throttle.min_downloads"), rpc::make_target(download));
  rpc::call_command("d.downloads_max.set",    rpc::call_command("throttle.max_downloads"), rpc::make_target(download));
  rpc::call_command("d.peers_min.set",        rpc::call_command("throttle.min_peers.normal"), rpc::make_target(download));
  rpc::call_command("d.peers_max.set",        rpc::call_command("throttle.max_peers.normal"), rpc::make_target(download));
  rpc::call_command("d.tracker_numwant.set",  rpc::call_command("trackers.numwant"), rpc::make_target(download));
  rpc::call_command("d.max_file_size.set",    rpc::call_command("system.file.max_size"), rpc::make_target(download));

  if (rpc::call_command_value("d.complete", rpc::make_target(download)) != 0) {
    if (rpc::call_command_value("throttle.min_peers.seed") >= 0)
      rpc::call_command("d.peers_min.set", rpc::call_command("throttle.min_peers.seed"), rpc::make_target(download));

    if (rpc::call_command_value("throttle.max_peers.seed") >= 0)
      rpc::call_command("d.peers_max.set", rpc::call_command("throttle.max_peers.seed"), rpc::make_target(download));
  }

  if (!rpc::call_command_value("trackers.use_udp"))
    download->enable_udp_trackers(false);

  // Check first if we already have these values set in the session
  // torrent, so that it is safe to change the values.
  //
  // Need to also catch the exceptions.
  if (rpc::call_command_value("system.file.split_size") >= 0)
    torrent::file_split_all(download->download()->file_list(),
                            rpc::call_command_value("system.file.split_size"),
                            rpc::call_command_string("system.file.split_suffix"));

  if (!rtorrent->has_key_string("directory"))
    rpc::call_command("d.directory.set", m_variables["directory"], rpc::make_target(download));
  else
    rpc::call_command("d.directory_base.set", rtorrent->get_key("directory"), rpc::make_target(download));

  if (!m_session && m_variables["tied_to_file"].as_value())
    rpc::call_command("d.tied_to_file.set", m_uri.empty() ? m_variables["tied_file"] : m_uri, rpc::make_target(download));

  rpc::call_command("d.peer_exchange.set", rpc::call_command_value("protocol.pex"), rpc::make_target(download));

  torrent::resume_load_addresses(*download->download(), resumeObject);
  torrent::resume_load_file_priorities(*download->download(), resumeObject);
  torrent::resume_load_tracker_settings(*download->download(), resumeObject);

  // The action of inserting might cause the torrent to be
  // opened/started or such. Figure out a nicer way of handling this.
  if (m_manager->download_list()->insert(download) == m_manager->download_list()->end()) {
    // ATM doesn't really ever get here.
    delete download;

    m_slot_finished();
    return;
  }

  // Save the info-hash just in case the commands decide to delete it.
  torrent::HashString infohash = download->info()->hash();

  try {
    if (torrent::log_groups[torrent::LOG_TORRENT_DEBUG].valid())
      log_created(download, rtorrent);

    std::for_each(m_commands.begin(), m_commands.end(),
                  rak::bind2nd(std::ptr_fun(&rpc::parse_command_multiple_std), rpc::make_target(download)));

    if (m_manager->download_list()->find(infohash) == m_manager->download_list()->end())
      throw torrent::input_error("The newly created download was removed.");

    if (!m_session)
       rpc::call_command("d.state.set", (int64_t)m_start, rpc::make_target(download));

    rpc::commands.call_catch(m_session ? "event.download.inserted_session" : "event.download.inserted_new",
                             rpc::make_target(download), torrent::Object(), "Download event action failed: ");

  } catch (torrent::input_error& e) {
    std::string msg = "Command on torrent creation failed: " + std::string(e.what());

    if (m_printLog)
      m_manager->push_log_std(msg);
    
    if (m_manager->download_list()->find(infohash) != m_manager->download_list()->end()) {
      // Should stop it, mark it bad. Perhaps even delete it?
      download->set_hash_failed(true);
      download->set_message(msg);
      //     m_manager->download_list()->erase(m_manager->download_list()->find(infohash.data()));
    }
  }

  m_slot_finished();
}

void
DownloadFactory::log_created(Download* download, torrent::Object* rtorrent) {
  std::stringstream dump;

  dump << "info_hash = " << torrent::hash_string_to_hex_str(download->info()->hash()) << std::endl;
  dump << "session = " << (m_session ? "true" : "false") << std::endl;

  if (download->download()->info()->is_meta_download())
    dump << "magnet = true" << std::endl;

  if (!rtorrent->has_key_string("directory"))
    dump << "directory = \"" << (m_variables["directory"].is_string() ? m_variables["directory"].as_string() : std::string()) << '"' << std::endl;
  else
    dump << "directory_base = \"" << (rtorrent->get_key("directory").is_string() ? rtorrent->get_key("directory").as_string() : std::string()) << '"' << std::endl;

  dump << "---COMMANDS---" << std::endl;

  for (command_list_type::const_iterator itr = m_commands.begin(); itr != m_commands.end(); itr++) {
    dump << *itr << std::endl;
  }

  std::string dump_str = dump.str();

  lt_log_print_dump(torrent::LOG_TORRENT_DEBUG, dump_str.c_str(), dump_str.size(), "Creating new download:");
}

void
DownloadFactory::receive_failed(const std::string& msg) {
  // Add message to log.
  if (m_printLog)
    m_manager->push_log_std(msg + ": \"" + m_uri + "\"");

  m_slot_finished();
}

void
DownloadFactory::initialize_rtorrent(Download* download, torrent::Object* rtorrent) {
  if (!rtorrent->has_key_value("state") || rtorrent->get_key_value("state") > 1) {
    rtorrent->insert_key("state", (int64_t)m_start);
    rtorrent->insert_key("state_changed", cachedTime.seconds());
    rtorrent->insert_key("state_counter", int64_t());

  } else if (!rtorrent->has_key_value("state_changed") ||
             rtorrent->get_key_value("state_changed") > cachedTime.seconds() || rtorrent->get_key_value("state_changed") == 0 ||
             !rtorrent->has_key_value("state_counter") || (uint64_t)rtorrent->get_key_value("state_counter") > (1 << 20)) {
    rtorrent->insert_key("state_changed", cachedTime.seconds());
    rtorrent->insert_key("state_counter", int64_t());
  }

  rtorrent->insert_preserve_copy("complete", (int64_t)0);
  rtorrent->insert_preserve_copy("hashing",  (int64_t)Download::variable_hashing_stopped);

  rtorrent->insert_preserve_copy("timestamp.started",  (int64_t)0);
  rtorrent->insert_preserve_copy("timestamp.finished", (int64_t)0);

  rtorrent->insert_preserve_copy("tied_to_file", "");
  rtorrent->insert_key("loaded_file", m_isFile ? m_uri : std::string());

  if (rtorrent->has_key_value("priority"))
    rpc::call_command("d.priority.set", rtorrent->get_key_value("priority") % 4, rpc::make_target(download));
  else
    rpc::call_command("d.priority.set", (int64_t)2, rpc::make_target(download));

  if (rtorrent->has_key_value("key")) {
    download->tracker_list()->set_key(rtorrent->get_key_value("key"));

  } else {
    download->tracker_list()->set_key(random() % (std::numeric_limits<uint32_t>::max() - 1) + 1);
    rtorrent->insert_key("key", download->tracker_list()->key());
  }

  if (rtorrent->has_key_value("total_uploaded"))
    download->info()->mutable_up_rate()->set_total(rtorrent->get_key_value("total_uploaded"));

  if (rtorrent->has_key_value("chunks_done") && rtorrent->has_key_value("chunks_wanted"))
    download->download()->set_chunks_done(rtorrent->get_key_value("chunks_done"), rtorrent->get_key_value("chunks_wanted"));

  download->set_throttle_name(rtorrent->has_key_string("throttle_name")
                              ? rtorrent->get_key_string("throttle_name")
                              : std::string());

  rtorrent->insert_preserve_copy("ignore_commands", (int64_t)0);
  rtorrent->insert_preserve_copy("views", torrent::Object::create_list());

  rtorrent->insert_preserve_type("connection_leech", m_variables["connection_leech"]);
  rtorrent->insert_preserve_type("connection_seed",  m_variables["connection_seed"]);

  rtorrent->insert_preserve_copy("choke_heuristics.up.leech",   std::string());
  rtorrent->insert_preserve_copy("choke_heuristics.up.seed",    std::string());
  rtorrent->insert_preserve_copy("choke_heuristics.down.leech", std::string());
  rtorrent->insert_preserve_copy("choke_heuristics.down.seed",  std::string());
}

}
