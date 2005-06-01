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

#ifndef RTORRENT_OPTION_FILE_H
#define RTORRENT_OPTION_FILE_H

#include <iosfwd>
#include <string>
#include <sigc++/slot.h>

class OptionFile {
public:
  static const int max_size_key = 64;
  static const int max_size_opt = 512;
  static const int max_size_line = max_size_key + max_size_opt + 64;

  typedef sigc::slot2<void, const std::string&, const std::string&> SlotStringPair;
  
  void                slot_option(const SlotStringPair& s) { m_slotOption = s; }

  void                process(std::istream* stream);

private:
  void                parse_line(const char* line);

  SlotStringPair      m_slotOption;
};

#endif
