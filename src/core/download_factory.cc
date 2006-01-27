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

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <torrent/bencode.h>
#include <torrent/exceptions.h>

#include "utils/variable_generic.h"

#include "curl_get.h"
#include "http_queue.h"
#include "globals.h"
#include "manager.h"

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

  m_variables.insert("connection_leech", new utils::VariableAny(control->variables()->get("connection_leech")));
  m_variables.insert("connection_seed",  new utils::VariableAny(control->variables()->get("connection_seed")));
  m_variables.insert("directory",        new utils::VariableAny(control->variables()->get("directory")));
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
    HttpQueue::iterator itr = m_manager->get_http_queue().insert(m_uri, m_stream);

    (*itr)->signal_done().slots().push_front(sigc::mem_fun(*this, &DownloadFactory::receive_loaded));
    (*itr)->signal_failed().slots().push_front(sigc::mem_fun(*this, &DownloadFactory::receive_failed));

    m_variables.set("tied_to_file", (int64_t)false);

  } else {
    m_stream = new std::fstream(m_uri.c_str(), std::ios::in);

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

  Manager::DListItr itr = m_manager->insert(m_stream, m_printLog);

  if (itr == m_manager->get_download_list().end()) {
    // core::Manager should already have added the error message to
    // the log.
    m_slotFinished();
    return;
  }

  torrent::Bencode& root = (*itr)->get_bencode();

  if (!m_session) {
    // We only allow session torrents to keep their
    // 'rtorrent/libtorrent' sections.
    root.erase_key("rtorrent");
    root.erase_key("libtorrent");
  }

  if (!root.has_key("rtorrent") ||
      !root.get_key("rtorrent").is_map())
    root.insert_key("rtorrent", torrent::Bencode(torrent::Bencode::TYPE_MAP));
    
  torrent::Bencode& rtorrent = root.get_key("rtorrent");

  if (!rtorrent.has_key("state") ||
      !rtorrent.get_key("state").is_string() ||
      (rtorrent.get_key("state").as_string() != "stopped" &&
       rtorrent.get_key("state").as_string() != "started"))
    rtorrent.insert_key("state", "stopped");
  
  if (!rtorrent.has_key("tied_to_file") ||
      !rtorrent.get_key("tied_to_file").is_string())
    rtorrent.insert_key("tied_to_file", std::string());

  if (rtorrent.has_key("priority") &&
      rtorrent.get_key("priority").is_value())
    (*itr)->variables()->set("priority", rtorrent.get_key("priority").as_value() % 4);
  else
    (*itr)->variables()->set("priority", (int64_t)2);

  // Move to 'rtorrent'.
  (*itr)->variables()->set("connection_leech", m_variables.get("connection_leech"));
  (*itr)->variables()->set("connection_seed",  m_variables.get("connection_seed"));
  (*itr)->variables()->set("min_peers",        control->variables()->get("min_peers"));
  (*itr)->variables()->set("max_peers",        control->variables()->get("max_peers"));
  (*itr)->variables()->set("max_uploads",      control->variables()->get("max_uploads"));

  if (control->variables()->get("use_udp_trackers").as_string() == "no")
    (*itr)->enable_udp_trackers(false);

  if (m_session) {
    if (!rtorrent.has_key("directory") ||
	!rtorrent.get_key("directory").is_string())
      (*itr)->variables()->set("directory", m_variables.get("directory"));
    else
      (*itr)->variables()->set("directory", rtorrent.get_key("directory"));

    if ((*itr)->variables()->get_string("state") == "started")
      m_manager->start(*itr, m_printLog);

  } else {
    (*itr)->variables()->set("directory", m_variables.get("directory"));

    if (m_variables.get("tied_to_file").as_value())
      (*itr)->variables()->set("tied_to_file", m_uri);

    if (m_start)
      m_manager->start(*itr, m_printLog);

    m_manager->get_download_store().save(*itr);
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
