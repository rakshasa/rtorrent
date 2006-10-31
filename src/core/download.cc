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
#include <rak/path.h>
#include <torrent/exceptions.h>
#include <torrent/rate.h>
#include <torrent/torrent.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>

#include "utils/variable_generic.h"

#include "download.h"

namespace core {

Download::Download(download_type d) :
  m_download(d),
  m_fileList(d.file_list()),
  m_trackerList(d.tracker_list()),

  m_hashFailed(false),

  m_chunksFailed(0) {

  m_connTrackerSucceded = m_download.signal_tracker_succeded(sigc::bind(sigc::mem_fun(*this, &Download::receive_tracker_msg), ""));
  m_connTrackerFailed   = m_download.signal_tracker_failed(sigc::mem_fun(*this, &Download::receive_tracker_msg));
  m_connStorageError    = m_download.signal_storage_error(sigc::mem_fun(*this, &Download::receive_storage_error));

  m_download.signal_chunk_failed(sigc::mem_fun(*this, &Download::receive_chunk_failed));

  m_variables.insert("connection_current", new utils::VariableStringSlot(rak::mem_fn(this, &Download::connection_current),
                                                                         rak::mem_fn(this, &Download::set_connection_current)));

  m_variables.insert("connection_leech",   new utils::VariableAny(connection_type_to_string(download_type::CONNECTION_LEECH)));
  m_variables.insert("connection_seed",    new utils::VariableAny(connection_type_to_string(download_type::CONNECTION_SEED)));

  // 0 - stopped
  // 1 - started
  m_variables.insert("state",              new utils::VariableObject(bencode(), "rtorrent", "state", torrent::Object::TYPE_VALUE));
  m_variables.insert("complete",           new utils::VariableObject(bencode(), "rtorrent", "complete", torrent::Object::TYPE_VALUE));

  // 0 - Not hashing
  // 1 - Normal hashing
  // 2 - Download finished, hashing
  m_variables.insert("hashing",            new utils::VariableObject(bencode(), "rtorrent", "hashing", torrent::Object::TYPE_VALUE));
  m_variables.insert("tied_to_file",       new utils::VariableObject(bencode(), "rtorrent", "tied_to_file", torrent::Object::TYPE_STRING));

  // The "state_changed" variable is required to be a valid unix time
  // value, it indicates the last time the torrent changed its state,
  // resume/pause.
  m_variables.insert("state_changed",      new utils::VariableObject(bencode(), "rtorrent", "state_changed", torrent::Object::TYPE_VALUE));

  m_variables.insert("directory",          new utils::VariableStringSlot(rak::mem_fn(&m_fileList, &torrent::FileList::root_dir), rak::mem_fn(this, &Download::set_root_directory)));

//   m_variables.insert("info_hash",          new utils::VariableStringSlot(rak::mem_fn(&m_download, &torrent::Download::info_hash), NULL));

  m_variables.insert("min_peers",          new utils::VariableValueSlot(rak::mem_fn(&m_download, &download_type::peers_min), rak::mem_fn(&m_download, &download_type::set_peers_min)));
  m_variables.insert("max_peers",          new utils::VariableValueSlot(rak::mem_fn(&m_download, &download_type::peers_max), rak::mem_fn(&m_download, &download_type::set_peers_max)));
  m_variables.insert("max_uploads",        new utils::VariableValueSlot(rak::mem_fn(&m_download, &download_type::uploads_max), rak::mem_fn(&m_download, &download_type::set_uploads_max)));

  m_variables.insert("up_rate",            new utils::VariableValueSlot(rak::mem_fn(m_download.up_rate(), &torrent::Rate::rate), NULL));
  m_variables.insert("up_total",           new utils::VariableValueSlot(rak::mem_fn(m_download.up_rate(), &torrent::Rate::total), NULL));
  m_variables.insert("down_rate",          new utils::VariableValueSlot(rak::mem_fn(m_download.down_rate(), &torrent::Rate::rate), NULL));
  m_variables.insert("down_total",         new utils::VariableValueSlot(rak::mem_fn(m_download.down_rate(), &torrent::Rate::total), NULL));
  m_variables.insert("skip_rate",          new utils::VariableValueSlot(rak::mem_fn(m_download.skip_rate(), &torrent::Rate::rate), NULL));
  m_variables.insert("skip_total",         new utils::VariableValueSlot(rak::mem_fn(m_download.skip_rate(), &torrent::Rate::total), NULL));

  m_variables.insert("priority",           new utils::VariableValueSlot(rak::mem_fn(this, &Download::priority), rak::mem_fn(this, &Download::set_priority)));

  m_variables.insert("tracker_numwant",    new utils::VariableValueSlot(rak::mem_fn(&m_trackerList, &tracker_list_type::numwant), rak::mem_fn(&m_trackerList, &tracker_list_type::set_numwant)));

  m_variables.insert("ignore_commands",    new utils::VariableObject(bencode(), "rtorrent", "ignore_commands", torrent::Object::TYPE_VALUE));
}

Download::~Download() {
  if (!m_download.is_valid())
    return;

  m_connTrackerSucceded.disconnect();
  m_connTrackerFailed.disconnect();
  m_connStorageError.disconnect();

  m_download = download_type();
}

void
Download::enable_udp_trackers(bool state) {
  torrent::TrackerList tl = m_download.tracker_list();

  for (int i = 0, last = tl.size(); i < last; ++i)
    if (tl.get(i).tracker_type() == torrent::Tracker::TRACKER_UDP)
      if (state)
        tl.get(i).enable();
      else
        tl.get(i).disable();
}

uint32_t
Download::priority() {
  return bencode()->get_key("rtorrent").get_key_value("priority");
}

void
Download::set_priority(uint32_t p) {
  if (p >= 4)
    throw torrent::input_error("Priority out of range.");

  // Seeding torrents get half the priority of unfinished torrents.
  if (!is_done())
    torrent::download_set_priority(m_download, p * p * 2);
  else
    torrent::download_set_priority(m_download, p * p);

  bencode()->get_key("rtorrent").insert_key("priority", (int64_t)p);
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

Download::download_type::ConnectionType
Download::string_to_connection_type(const std::string& name) {
  // Return default if the name isn't found.
  if (name == "leech")
    return download_type::CONNECTION_LEECH;
  else if (name == "seed")
    return download_type::CONNECTION_SEED;
  else
    throw torrent::input_error("Unknown peer connection type selected: \"" + name + "\"");
}

const char*
Download::connection_type_to_string(download_type::ConnectionType t) {
  switch (t) {
  case download_type::CONNECTION_LEECH:
    return "leech";
  case download_type::CONNECTION_SEED:
    return "seed";
  default:
    return "unknown";
  }
}

uint32_t
Download::string_to_priority(const std::string& name) {
  if (name == "off")
    return 0;
  else if (name == "low")
    return 1;
  else if (name == "normal")
    return 2;
  else if (name == "high")
    return 3;
  else
    throw torrent::input_error("Could not convert string to priority.");
}

const char*
Download::priority_to_string(uint32_t p) {
  switch (p) {
  case 0:
    return "off";
  case 1:
    return "low";
  case 2:
    return "normal";
  case 3:
    return "high";
  default:
    throw torrent::input_error("Priority out of range.");
  }
}

float
Download::distributed_copies() const {
  const uint8_t* avail = m_download.chunks_seen();
  const uint8_t* end = avail + m_download.chunks_total();

  if (avail == NULL)
    return 0;

  int minAvail = std::numeric_limits<uint8_t>::max();
  int num = 0;

  for (; avail < end; ++avail)
    if (*avail == minAvail) {
      num++;
    } else if (*avail < minAvail) {
      minAvail = *avail;
      num = 1;
    }

  return minAvail + 1 - (float)num / m_download.chunks_total();
}

void
Download::receive_chunk_failed(__UNUSED uint32_t idx) {
  m_chunksFailed++;
}

// Clean up.
void
Download::set_root_directory(const std::string& path) {
  if (path.empty()) {
    m_fileList.set_root_dir("./" + (m_fileList.size() > 1 ? m_download.name() : std::string()));

  } else {
    std::string fullPath = rak::path_expand(path);

    m_fileList.set_root_dir(fullPath +
			    (*fullPath.rbegin() != '/' ? "/" : "") +
			    (m_fileList.size() > 1 ? m_download.name() : ""));
  }

  bencode()->get_key("rtorrent").insert_key("directory", path);
}

}
