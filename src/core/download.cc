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
#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <sigc++/signal.h>
#include <torrent/exceptions.h>

#include "download.h"

namespace core {

Download::Download(torrent::Download d) :
  m_download(d),

  m_chunksFailed(0),
  m_connectionLeech(torrent::Download::CONNECTION_LEECH),
  m_connectionSeed(torrent::Download::CONNECTION_SEED) {

  m_connTrackerSucceded = m_download.signal_tracker_succeded(sigc::bind(sigc::mem_fun(*this, &Download::receive_tracker_msg), ""));
  m_connTrackerFailed = m_download.signal_tracker_failed(sigc::mem_fun(*this, &Download::receive_tracker_msg));
  m_connStorageError = m_download.signal_storage_error(sigc::mem_fun(*this, &Download::receive_storage_error));

  m_download.signal_chunk_failed(sigc::mem_fun(*this, &Download::receive_chunk_failed));
}

Download::~Download() {
  if (!m_download.is_valid())
    return;

  m_connTrackerSucceded.disconnect();
  m_connTrackerFailed.disconnect();
  m_connStorageError.disconnect();

  m_download = torrent::Download();
}

void
Download::start() {
  if (is_done()) {
    m_download.set_connection_type(m_connectionSeed);
    torrent::download_set_priority(m_download, 2);
  } else {
    m_download.set_connection_type(m_connectionLeech);
    torrent::download_set_priority(m_download, 4);
  }

  m_download.start();
}

void
Download::set_root_directory(const std::string& d) {
  m_download.set_root_dir(d +
			  (!d.empty() && *d.rbegin() != '/' ? "/" : "") +
			  (m_download.size_file_entries() > 1 ? m_download.name() : ""));
}

void
Download::enable_udp_trackers(bool state) {
  for (int i = 0, last = m_download.size_trackers(); i < last; ++i)
    if (m_download.tracker(i).tracker_type() == torrent::Tracker::TRACKER_UDP)
      if (state)
	m_download.tracker(i).enable();
      else
	m_download.tracker(i).disable();
}

void
Download::receive_finished() {
  m_download.set_connection_type(m_connectionSeed);
  torrent::download_set_priority(m_download, 2);
}

void
Download::receive_tracker_msg(std::string msg) {
  if (msg.empty())
    m_message = "";
  else
    m_message = "Tracker: [" + msg + "]";
}

void
Download::receive_storage_error(std::string msg) {
  m_message = "Storage error: [" + msg + "]";
}

torrent::Download::ConnectionType
Download::string_to_connection_type(const std::string& name) {
  // Return default if the name isn't found.
  if (name == "leech")
    return torrent::Download::CONNECTION_LEECH;
  else if (name == "seed")
    return torrent::Download::CONNECTION_SEED;
  else
    throw torrent::input_error("Unknown peer connection type selected: \"" + name + "\"");
}

const char*
Download::connection_type_to_string(torrent::Download::ConnectionType t) {
  switch (t) {
  case torrent::Download::CONNECTION_LEECH:
    return "leech";
  case torrent::Download::CONNECTION_SEED:
    return "seed";
  default:
    return "unknown";
  }
}

void
Download::receive_chunk_failed(uint32_t idx) {
  m_chunksFailed++;
}

}
