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

#ifndef RTORRENT_OPTION_PARSER_H
#define RTORRENT_OPTION_PARSER_H

#include <map>
#include <string>
#include <sigc++/slot.h>

// Throws std::runtime_error upon receiving bad input.

class OptionParser {
public:
  typedef sigc::slot0<void>                     SlotFlag;
  typedef sigc::slot1<void, const std::string&> SlotOption;
  typedef sigc::slot2<void, int, int>           SlotIntPair;

  struct Node {
    SlotOption m_slot;
    bool       m_useOption;
  };

  typedef std::map<char, Node>                  Container;

  void        insert_flag(char c, SlotFlag s);
  void        insert_option(char c, SlotOption s);

  // Returns the index of the first non-option argument.
  int         process(int argc, char** argv);

  static void call_int_pair(const std::string& str, SlotIntPair slot);

private:
  std::string create_optstring();

  void        call_flag(char c, const std::string& arg);

  Container   m_container;
};

#endif
