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

#ifndef RTORRENT_INPUT_PATH_INPUT_H
#define RTORRENT_INPUT_PATH_INPUT_H

#include "utils/directory.h"

#include "text_input.h"

namespace input {

class PathInput : public TextInput {
public:
  typedef std::pair<utils::Directory::iterator, utils::Directory::iterator> Range;

  PathInput();
  virtual ~PathInput() {}

  virtual bool        pressed(int key);

private:
  void                receive_do_complete();

  size_type           find_last_delim();
  Range               find_incomplete(utils::Directory& d, const std::string& f);
};

}

#endif
