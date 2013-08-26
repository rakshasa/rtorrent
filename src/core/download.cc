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

#include <list>
#include <rak/file_stat.h>
#include <rak/functional.h>
#include <rak/path.h>
#include <torrent/exceptions.h>
#include <torrent/rate.h>
#include <torrent/torrent.h>
#include <torrent/tracker.h>
#include <torrent/tracker_list.h>
#include <torrent/data/file_list.h>

#include "rpc/parse_commands.h"

#include "control.h"
#include "download.h"
#include "manager.h"

namespace core {

Download::Download(download_type d) :
  m_download(d),
  m_hashFailed(false),

  m_resumeFlags(~uint32_t()),
  m_group(0) {

  m_download.info()->signal_tracker_success().push_back(tr1::bind(&Download::receive_tracker_msg, this, ""));
  m_download.info()->signal_tracker_failed().push_back(tr1::bind(&Download::receive_tracker_msg, this, tr1::placeholders::_1));
}

Download::~Download() {
  if (!m_download.is_valid())
    return;

  m_download = download_type();
}

void
Download::enable_udp_trackers(bool state) {
  for (torrent::TrackerList::iterator itr = m_download.tracker_list()->begin(), last = m_download.tracker_list()->end(); itr != last; ++itr)
    if ((*itr)->type() == torrent::Tracker::TRACKER_UDP) {
      if (state)
        (*itr)->enable();
      else
        (*itr)->disable();
    }
}

uint32_t
Download::priority() {
  return bencode()->get_key("rtorrent").get_key_value("priority");
}

void
Download::set_priority(uint32_t p) {
  p %= 4;

  // Seeding torrents get half the priority of unfinished torrents.
  if (!is_done())
    torrent::download_set_priority(m_download, p * p * 2);
  else
    torrent::download_set_priority(m_download, p * p);

  bencode()->get_key("rtorrent").insert_key("priority", (int64_t)p);
}

uint32_t
Download::connection_list_size() const {
  return m_download.connection_list()->size();
}

void
Download::receive_tracker_msg(std::string msg) {
  if (msg.empty())
    m_message = "";
  else
    m_message = "Tracker: [" + msg + "]";
}

float
Download::distributed_copies() const {
  const uint8_t* avail = m_download.chunks_seen();
  const torrent::Bitfield* bitfield = m_download.file_list()->bitfield();

  if (avail == NULL)
    return 0;

  int minAvail = std::numeric_limits<uint8_t>::max();
  int num = 0;

  for (uint32_t i = m_download.file_list()->size_chunks(); i-- > 0; ) {
    int totAvail = (int)avail[i] + bitfield->get(i);
    if (totAvail == minAvail) {
      num++;
    } else if (totAvail < minAvail) {
      minAvail = totAvail;
      num = 1;
    }
  }

  return minAvail + 1 - bitfield->is_all_set() - (float)num / m_download.file_list()->size_chunks();
}

void
Download::set_throttle_name(const std::string& throttleName) {
  if (m_download.info()->is_active())
    throw torrent::input_error("Cannot set throttle on active download.");

  torrent::ThrottlePair throttles = control->core()->get_throttle(throttleName);
  m_download.set_upload_throttle(throttles.first);
  m_download.set_download_throttle(throttles.second);

  m_download.bencode()->get_key("rtorrent").insert_key("throttle_name", throttleName);
}

void
Download::set_root_directory(const std::string& path) {
  // If the download is open, hashed and has completed chunks make
  // sure to verify that the download files are still present.
  // 
  // This should ensure that no one tries to set the destination
  // directory 'after' moving files. In cases where the user wants to
  // override this behavior the download must first be closed or
  // 'd.directory_base.set' may be used.
  rak::file_stat file_stat;
  torrent::FileList* file_list = m_download.file_list();

  if (is_hash_checked() && file_list->completed_chunks() != 0 &&

      (file_list->is_multi_file() ?
       !file_list->is_root_dir_created() :
       !file_stat.update(file_list->front()->frozen_path()))) {

    set_message("Cannot change the directory of an open download after the files have been moved.");
    rpc::call_command("d.state.set", (int64_t)0, rpc::make_target(this));
    control->core()->download_list()->close_directly(this);

    throw torrent::input_error("Cannot change the directory of an open download atter the files have been moved.");
  }

  control->core()->download_list()->close_directly(this);
  file_list->set_root_dir(rak::path_expand(path));

  bencode()->get_key("rtorrent").insert_key("directory", path);
}

}
