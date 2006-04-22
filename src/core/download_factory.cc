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

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <rak/path.h>
#include <torrent/object.h>
#include <torrent/exceptions.h>
#include <torrent/rate.h>

#include "utils/variable_generic.h"

#include "curl_get.h"
#include "http_queue.h"
#include "globals.h"
#include "manager.h"

#include "download.h"
#include "download_factory.h"

namespace core {

DownloadFactory::DownloadFactory(const std::string& uri, Manager* m) :
  m_manager(m),
  m_stream(NULL),
  m_commited(false),
  m_loaded(false),

  m_uri(uri),
  m_session(false),
  m_start(false),
  m_printLog(true) {

  m_taskLoad.set_slot(rak::mem_fn(this, &DownloadFactory::receive_load));
  m_taskCommit.set_slot(rak::mem_fn(this, &DownloadFactory::receive_commit));

  m_variables.insert("connection_leech", new utils::VariableAny(control->variable()->get("connection_leech")));
  m_variables.insert("connection_seed",  new utils::VariableAny(control->variable()->get("connection_seed")));
  m_variables.insert("directory",        new utils::VariableAny(control->variable()->get("directory")));
  m_variables.insert("tied_to_file",     new utils::VariableBool(false));
}

DownloadFactory::~DownloadFactory() {
  priority_queue_erase(&taskScheduler, &m_taskLoad);
  priority_queue_erase(&taskScheduler, &m_taskCommit);

  delete m_stream;
  m_stream = NULL;
}

void
DownloadFactory::load() {
  priority_queue_insert(&taskScheduler, &m_taskLoad, cachedTime);
}

void
DownloadFactory::commit() {
  priority_queue_insert(&taskScheduler, &m_taskCommit, cachedTime);
}

void
DownloadFactory::receive_load() {
  if (m_stream)
    throw torrent::client_error("DownloadFactory::load() called on an object with m_stream != NULL");

  if (std::strncmp(m_uri.c_str(), "http://", 7) == 0) {
    // Http handling here.
    m_stream = new std::stringstream;
    HttpQueue::iterator itr = m_manager->http_queue().insert(m_uri, m_stream);

    (*itr)->signal_done().slots().push_front(sigc::mem_fun(*this, &DownloadFactory::receive_loaded));
    (*itr)->signal_failed().slots().push_front(sigc::mem_fun(*this, &DownloadFactory::receive_failed));

    m_variables.set("tied_to_file", (int64_t)false);

  } else {
    m_stream = new std::fstream(rak::path_expand(m_uri).c_str(), std::ios::in | std::ios::binary);

    if (m_stream->good())
      receive_loaded();
    else
      receive_failed("Could not open file");
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
    throw torrent::client_error("DownloadFactory::receive_success() called on an object with m_stream == NULL");

  DownloadList::iterator itr = m_manager->download_list().insert(m_stream, m_printLog);

  if (itr == m_manager->download_list().end()) {
    // core::Manager should already have added the error message to
    // the log.
    m_slotFinished();
    return;
  }

  torrent::Object* root = (*itr)->bencode();

  if (!m_session) {
    // We only allow session torrents to keep their
    // 'rtorrent/libtorrent' sections.
    root->erase_key("rtorrent");
    root->erase_key("libtorrent");
  }

  if (!root->has_key_map("rtorrent"))
    root->insert_key("rtorrent", torrent::Object(torrent::Object::TYPE_MAP));
    
  torrent::Object* rtorrent = &root->get_key("rtorrent");

  if (!rtorrent->has_key_string("state") ||
      (rtorrent->get_key("state").as_string() != "stopped" &&
       rtorrent->get_key("state").as_string() != "started"))
    rtorrent->insert_key("state", "stopped");
  
  if (!rtorrent->has_key_string("tied_to_file"))
    rtorrent->insert_key("tied_to_file", std::string());

  if (rtorrent->has_key_value("priority"))
    (*itr)->variable()->set("priority", rtorrent->get_key("priority").as_value() % 4);
  else
    (*itr)->variable()->set("priority", (int64_t)2);

  // Move to 'rtorrent'.
  (*itr)->variable()->set("connection_leech", m_variables.get("connection_leech"));
  (*itr)->variable()->set("connection_seed",  m_variables.get("connection_seed"));
  (*itr)->variable()->set("min_peers",        control->variable()->get("min_peers"));
  (*itr)->variable()->set("max_peers",        control->variable()->get("max_peers"));
  (*itr)->variable()->set("max_uploads",      control->variable()->get("max_uploads"));

  if (rtorrent->has_key_value("key")) {
    (*itr)->tracker_list()->set_key(rtorrent->get_key("key").as_value());

  } else {
    (*itr)->tracker_list()->set_key(rand() % (std::numeric_limits<uint32_t>::max() - 1) + 1);
    rtorrent->insert_key("key", (*itr)->tracker_list()->key());
  }

  if (!control->variable()->get_value("use_udp_trackers"))
    (*itr)->enable_udp_trackers(false);

  if (rtorrent->has_key_value("total_uploaded"))
    (*itr)->download()->up_rate()->set_total(rtorrent->get_key("total_uploaded").as_value());
    
  if (m_session) {
    if (!rtorrent->has_key_string("directory"))
      (*itr)->variable()->set("directory", m_variables.get("directory"));
    else
      (*itr)->variable()->set("directory", rtorrent->get_key("directory"));

    if ((*itr)->variable()->get_string("state") == "started")
      m_manager->download_list().resume(*itr);

  } else {
    (*itr)->variable()->set("directory", m_variables.get("directory"));

    if (m_variables.get("tied_to_file").as_value())
      (*itr)->variable()->set("tied_to_file", m_uri);

    if (m_start)
      m_manager->download_list().start(*itr);

    m_manager->download_store().save(*itr);
  }

  m_slotFinished();
}

void
DownloadFactory::receive_failed(const std::string& msg) {
  if (m_stream == NULL)
    throw torrent::client_error("DownloadFactory::receive_success() called on an object with m_stream == NULL");

  // Add message to log.
  if (m_printLog) {
    m_manager->get_log_important().push_front(msg + ": \"" + m_uri + "\"");
    m_manager->get_log_complete().push_front(msg + ": \"" + m_uri + "\"");
  }

  m_slotFinished();
}

}
