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

#include <sigc++/bind.h>
#include <sigc++/hide.h>
#include <sigc++/signal.h>
#include <rak/path.h>
#include <rak/functional.h>
#include <torrent/exceptions.h>
#include <torrent/rate.h>
#include <torrent/torrent.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>
#include <torrent/data/file_list.h>

#include "rpc/command_map.h"

#include "control.h"
#include "download.h"

namespace core {

Download::Download(download_type d) :
  m_download(d),
  m_trackerList(d.tracker_list()),

  m_hashFailed(false),

  m_chunksFailed(0) {

  m_connTrackerSucceded = m_download.signal_tracker_succeded(sigc::bind(sigc::mem_fun(*this, &Download::receive_tracker_msg), ""));
  m_connTrackerFailed   = m_download.signal_tracker_failed(sigc::mem_fun(*this, &Download::receive_tracker_msg));
  m_connStorageError    = m_download.signal_storage_error(sigc::mem_fun(*this, &Download::receive_storage_error));

  m_download.signal_chunk_failed(sigc::mem_fun(*this, &Download::receive_chunk_failed));
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
  const uint8_t* end = avail + m_download.file_list()->size_chunks();

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

  return minAvail + 1 - (float)num / m_download.file_list()->size_chunks();
}

void
Download::receive_chunk_failed(__UNUSED uint32_t idx) {
  m_chunksFailed++;
}

int64_t            Download::get_value(const char* key)                            { return control->download_variables()->call_command_d_value(key, this); }
const std::string Download::get_string(const char* key)                           { return control->download_variables()->call_command_d_string(key, this); }
int64_t            Download::get_std_value(const std::string& key)                 { return control->download_variables()->call_command_d_value(key.c_str(), this); }
const std::string Download::get_std_string(const std::string& key)                { return control->download_variables()->call_command_d_string(key.c_str(), this); }

void               Download::set(const char* key, const torrent::Object& value)    { control->download_variables()->call_command_d(key, this, value); }
void               Download::set_value(const char* key, int64_t value)             { control->download_variables()->call_command_d_set_value(key, this, value); }
void               Download::set_string(const char* key, const std::string& value) { control->download_variables()->call_command_d_set_string(key, this, value); }

// Clean up.
void
Download::set_root_directory(const std::string& path) {
  torrent::FileList* fileList = m_download.file_list();

  if (path.empty()) {
    fileList->set_root_dir("./" + (fileList->size_files() > 1 ? m_download.name() : std::string()));

  } else {
    std::string fullPath = rak::path_expand(path);

    fileList->set_root_dir(fullPath +
                           (*fullPath.rbegin() != '/' ? "/" : "") +
                           (fileList->size_files() > 1 ? m_download.name() : ""));
  }

  bencode()->get_key("rtorrent").insert_key("directory", path);
}

}
