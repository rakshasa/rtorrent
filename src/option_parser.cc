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
#include <unistd.h>
#include <sigc++/bind.h>
#include <sigc++/hide.h>

#include "option_parser.h"

void
OptionParser::insert_flag(char c, Slot s) {
  m_container[c].m_slot = sigc::hide(s);
  m_container[c].m_useOption = false;
}

void
OptionParser::insert_option(char c, SlotString s) {
  m_container[c].m_slot = s;
  m_container[c].m_useOption = true;
}

void
OptionParser::insert_option_list(char c, SlotStringPair s) {
  m_container[c].m_slot = sigc::bind<0>(sigc::ptr_fun(&OptionParser::call_option_list), s);
  m_container[c].m_useOption = true;
}

void
OptionParser::insert_int_pair(char c, SlotIntPair s) {
  m_container[c].m_slot = sigc::bind<0>(sigc::ptr_fun(&OptionParser::call_int_pair), s);
  m_container[c].m_useOption = true;
}

int
OptionParser::process(int argc, char** argv) {
  int c;
  std::string optString = create_optstring();

  opterr = 0;

  while ((c = getopt(argc, argv, optString.c_str())) != -1)
    if (c == '?')
      throw std::runtime_error("Invalid/unknown option flag \"-" + std::string(1, (char)optopt) + "\". See rtorrent -h for more information.");
    else
      call(c, optarg ? optarg : "");

  return optind;
}

std::string
OptionParser::create_optstring() {
  std::string s;

  for (Container::iterator itr = m_container.begin(); itr != m_container.end(); ++itr) {
    s += itr->first;

    if (itr->second.m_useOption)
      s += ':';
  }

  return s;
}

void
OptionParser::call(char c, const std::string& arg) {
  Container::iterator itr = m_container.find(c);

  if (itr == m_container.end())
    throw std::logic_error("OptionParser::call_flag(...) could not find the flag");

  itr->second.m_slot(arg);
}

void
OptionParser::call_option_list(SlotStringPair slot, const std::string& arg) {
  std::string::const_iterator itr = arg.begin();

  while (itr != arg.end()) {
    std::string::const_iterator last = std::find(itr, arg.end(), ',');
    std::string::const_iterator opt = std::find(itr, last, '=');

    if (opt == itr || opt == last)
      throw std::logic_error("Invalid argument, \"" + arg + "\" should be \"key1=opt1,key2=opt2,...\"");

    slot(std::string(itr, opt), std::string(opt + 1, last));

    if (last == arg.end())
      break;

    itr = ++last;
  }
}

void
OptionParser::call_int_pair(SlotIntPair slot, const std::string& arg) {
  int a, b;

  if (std::sscanf(arg.c_str(), "%u-%u", &a, &b) != 2)
    throw std::runtime_error("Invalid argument, \"" + arg + "\" should be \"a-b\"");

  slot(a, b);
}
