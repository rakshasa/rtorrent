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

#include <stdexcept>
#include <sigc++/bind.h>
#include <sigc++/hide.h>

#include "option_handler.h"

OptionHandlerBase::~OptionHandlerBase() {
}

void
OptionHandler::insert(const std::string& key, OptionHandlerBase* opt) {
  iterator itr = find(key);

  if (itr == end()) {
    Base::insert(value_type(key, opt));
  } else {
    delete itr->second;
    itr->second = opt;
  }
}

void
OptionHandler::erase(const std::string& key) {
  iterator itr = find(key);

  if (itr == end())
    return;

  delete itr->second;
  Base::erase(itr);
}

void
OptionHandler::clear() {
  for (iterator itr = begin(), last = end(); itr != last; ++itr)
    delete itr->second;

  Base::clear();
}

void
OptionHandler::process(const std::string& key, const std::string& arg) const {
  const_iterator itr = find(key);

  if (itr == end())
    throw std::runtime_error("Could not find option key matching \"" + key + "\"");

  itr->second->process(key, arg);
}
