// rTorrent - BitTorrent client
// Copyright (C) 2005, Jari Sundell
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
// Contact:  Jari Sundell <jaris@ifi.uio.no>
//
//           Skomakerveien 33
//           3185 Skoppum, NORWAY

#ifndef RTORRENT_OPTION_HANDLER_RULES_H
#define RTORRENT_OPTION_HANDLER_RULES_H

#include "core/download_slot_map.h"

#include "option_handler.h"

class OptionHandlerDownloadMinPeers : public OptionHandlerBase {
public:
  OptionHandlerDownloadMinPeers(core::DownloadSlotMap* m) : m_map(m) {}
  ~OptionHandlerDownloadMinPeers();
  
  virtual void process(const std::string& key, const std::string& arg);
  
private:
  // Move these somewhere else? A seperate file with lots of different
  // setting appliers. Those would perform the nessesary checks
  // themselves.
  static void apply(core::Download* d, int arg) {
    d->get_download().set_peers_min(arg);
  }

  core::DownloadSlotMap* m_map;
};

#endif
