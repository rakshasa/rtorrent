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

#include "config.h"

#include <cstdio>
#include <stdexcept>
#include <sigc++/bind.h>

#include "option_handler_rules.h"

OptionHandlerDownloadMinPeers::~OptionHandlerDownloadMinPeers() {
}

void
OptionHandlerDownloadMinPeers::process(const std::string& key, const std::string& arg) {
  int a;

  if (std::sscanf(arg.c_str(), "%i", &a) != 1 ||
      a <= 0 ||
      a >= (1 << 16))
    throw std::runtime_error("Invalid argument for min peers \"" + arg + "\"");

  (*m_map)[key] = sigc::bind(sigc::ptr_fun(&OptionHandlerDownloadMinPeers::apply), a);
}
