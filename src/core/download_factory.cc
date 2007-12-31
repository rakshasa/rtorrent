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

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <rak/path.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>
#include <torrent/rate.h>
#include <torrent/resume.h>
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

DownloadFactory::DownloadFactory(Manager* m) :
  m_manager(m),
  m_stream(NULL),
  m_commited(false),
  m_loaded(false),

  m_session(false),
  m_start(false),
  m_printLog(true),
  m_isFile(false) {

  m_taskLoad.set_slot(rak::mem_fn(this, &DownloadFactory::receive_load));
  m_taskCommit.set_slot(rak::mem_fn(this, &DownloadFactory::receive_commit));

  m_variables["connection_leech"] = rpc::call_command_void("get_connection_leech");
  m_variables["connection_seed"]  = rpc::call_command_void("get_connection_seed");
  m_variables["directory"]        = rpc::call_command_void("get_directory");
  m_variables["tied_to_file"]     = torrent::Object((int64_t)false);
}

DownloadFactory::~DownloadFactory() {
  priority_queue_erase(&taskScheduler, &m_taskLoad);
  priority_queue_erase(&taskScheduler, &m_taskCommit);

  delete m_stream;
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

    (*itr)->signal_done().slots().push_front(sigc::mem_fun(*this, &DownloadFactory::receive_loaded));
    (*itr)->signal_failed().slots().push_front(sigc::mem_fun(*this, &DownloadFactory::receive_failed));

    m_variables["tied_to_file"] = (int64_t)false;

  } else {
    std::fstream* stream = new std::fstream(rak::path_expand(m_uri).c_str(), std::ios::in | std::ios::binary);
    m_stream = stream;

    // Since FileStatusCache checks file stats, it will automatically
    // cull away invalid paths.
    m_isFile = true;

    if (!stream->is_open())
      return receive_failed("Could not open file");

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
  if (m_stream == NULL)
    throw torrent::internal_error("DownloadFactory::receive_success() called on an object with m_stream == NULL.");

  Download* download = m_manager->download_list()->create(m_stream, m_printLog);

  if (download == NULL) {
    // core::Manager should already have added the error message to
    // the log.
    m_slotFinished();
    return;
  }

  torrent::Object* root = download->bencode();

  if (!m_session) {
    // We only allow session torrents to keep their
    // 'rtorrent/libtorrent' sections. The "fast_resume" section
    // should be safe to keep.
    root->erase_key("rtorrent");
    root->erase_key("libtorrent");
  }

  torrent::Object* rtorrent = &root->insert_preserve_map("rtorrent").first->second;

  initialize_rtorrent(download, rtorrent);

  if (!rtorrent->has_key_string("custom1")) rtorrent->insert_key("custom1", std::string());
  if (!rtorrent->has_key_string("custom2")) rtorrent->insert_key("custom2", std::string());
  if (!rtorrent->has_key_string("custom3")) rtorrent->insert_key("custom3", std::string());
  if (!rtorrent->has_key_string("custom4")) rtorrent->insert_key("custom4", std::string());
  if (!rtorrent->has_key_string("custom5")) rtorrent->insert_key("custom5", std::string());

  // Move to 'rtorrent'.
  rpc::call_command("d.set_connection_leech", m_variables["connection_leech"], rpc::make_target(download));
  rpc::call_command("d.set_connection_seed",  m_variables["connection_seed"], rpc::make_target(download));

  rpc::call_command("d.set_uploads_max",      rpc::call_command_void("get_max_uploads"), rpc::make_target(download));
  rpc::call_command("d.set_peers_min",        rpc::call_command_void("get_min_peers"), rpc::make_target(download));
  rpc::call_command("d.set_peers_max",        rpc::call_command_void("get_max_peers"), rpc::make_target(download));
  rpc::call_command("d.set_tracker_numwant",  rpc::call_command_void("get_tracker_numwant"), rpc::make_target(download));

  if (rpc::call_command_value("d.get_complete", rpc::make_target(download)) != 0) {
    if (rpc::call_command_value("get_min_peers_seed") >= 0)
      rpc::call_command("d.set_peers_min", rpc::call_command_void("get_min_peers_seed"), rpc::make_target(download));

    if (rpc::call_command_value("get_max_peers_seed") >= 0)
      rpc::call_command("d.set_peers_max", rpc::call_command_void("get_max_peers_seed"), rpc::make_target(download));
  }

  if (!rpc::call_command_value("get_use_udp_trackers"))
    download->enable_udp_trackers(false);

  if (rpc::call_command_value("get_max_file_size") > 0)
    rpc::call_command("d.set_max_file_size", rpc::call_command_void("get_max_file_size"), rpc::make_target(download));

  // Check first if we already have these values set in the session
  // torrent, so that it is safe to change the values.
  //
  // Need to also catch the exceptions.
  if (rpc::call_command_value("get_split_file_size") >= 0)
    torrent::file_split_all(download->download()->file_list(),
                            rpc::call_command_value("get_split_file_size"),
                            rpc::call_command_string("split_suffix"));

  if (!rtorrent->has_key_string("directory"))
    rpc::call_command("d.set_directory", m_variables["directory"], rpc::make_target(download));
  else
    rpc::call_command("d.set_directory_base", rtorrent->get_key("directory"), rpc::make_target(download));

  if (!m_session && m_variables["tied_to_file"].as_value())
    rpc::call_command("d.set_tied_to_file", m_uri, rpc::make_target(download));

  rpc::call_command("d.set_peer_exchange", rpc::call_command_value("get_peer_exchange"), rpc::make_target(download));

  torrent::Object& resumeObject = root->insert_preserve_map("libtorrent_resume").first->second;
  torrent::resume_load_addresses(*download->download(), resumeObject);
  torrent::resume_load_file_priorities(*download->download(), resumeObject);
  torrent::resume_load_tracker_settings(*download->download(), resumeObject);

  // The action of inserting might cause the torrent to be
  // opened/started or such. Figure out a nicer way of handling this.
  if (m_manager->download_list()->insert(download) == m_manager->download_list()->end()) {
    // ATM doesn't really ever get here.
    delete download;

    m_slotFinished();
    return;
  }

  // Save the info-hash just in case the commands decide to delete it.
  torrent::HashString infohash = download->download()->info_hash();

  try {
    std::for_each(m_commands.begin(), m_commands.end(), rak::bind1st(std::ptr_fun(&rpc::parse_command_d_multiple_std), download));

    if (m_manager->download_list()->find(infohash) == m_manager->download_list()->end())
      throw torrent::input_error("The newly created download was removed.");

  } catch (torrent::input_error& e) {
    std::string msg = "Command on torrent creation failed: " + std::string(e.what());

    if (m_printLog) {
      m_manager->get_log_important().push_front(msg);
      m_manager->get_log_complete().push_front(msg);
    }
    
    if (m_manager->download_list()->find(infohash) != m_manager->download_list()->end()) {
      // Should stop it, mark it bad. Perhaps even delete it?
      download->set_hash_failed(true);
      download->set_message(msg);
      //     m_manager->download_list()->erase(m_manager->download_list()->find(infohash.data()));
    }

    return m_slotFinished();
  }

  // When a download scheduler is implemented, this is handled by the
  // above insertion into download list.
  if (m_session) {
    // This torrent was queued for hashing or hashing when the session
    // file was saved. Or it was in a started state.
    if (rpc::call_command_value("d.get_hashing", rpc::make_target(download)) != Download::variable_hashing_stopped ||
        rpc::call_command_value("d.get_state", rpc::make_target(download)) != 0)
      m_manager->download_list()->resume(download);

  } else {
    // Use the state thingie here, move below.
    if (m_start)
      m_manager->download_list()->start_normal(download);

    m_manager->download_store()->save(download);
  }

  m_slotFinished();
}

void
DownloadFactory::receive_failed(const std::string& msg) {
  if (m_stream == NULL)
    throw torrent::internal_error("DownloadFactory::receive_success() called on an object with m_stream == NULL");

  // Add message to log.
  if (m_printLog) {
    m_manager->get_log_important().push_front(msg + ": \"" + m_uri + "\"");
    m_manager->get_log_complete().push_front(msg + ": \"" + m_uri + "\"");
  }

  m_slotFinished();
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

  rtorrent->insert_preserve_value("complete", 0);
  rtorrent->insert_preserve_value("hashing", Download::variable_hashing_stopped);
  rtorrent->insert_preserve_cstr("tied_to_file", "");

  if (rtorrent->has_key_value("priority"))
    rpc::call_command("d.set_priority", rtorrent->get_key_value("priority") % 4, rpc::make_target(download));
  else
    rpc::call_command("d.set_priority", (int64_t)2, rpc::make_target(download));

  if (rtorrent->has_key_value("key")) {
    download->tracker_list()->set_key(rtorrent->get_key_value("key"));

  } else {
    download->tracker_list()->set_key(random() % (std::numeric_limits<uint32_t>::max() - 1) + 1);
    rtorrent->insert_key("key", download->tracker_list()->key());
  }

  if (rtorrent->has_key_value("total_uploaded"))
    download->download()->up_rate()->set_total(rtorrent->get_key_value("total_uploaded"));

  if (rtorrent->has_key_value("chunks_done"))
    download->download()->set_chunks_done(std::min<uint32_t>(rtorrent->get_key_value("chunks_done"),
                                                             download->download()->file_list()->size_chunks()));

  rtorrent->insert_preserve_value("ignore_commands", 0);
}

}
