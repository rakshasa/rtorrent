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

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <functional>
#include <getopt.h>
#include <stdexcept>
#include <unistd.h>

#include "option_parser.h"

void
OptionParser::insert_flag(char c, slot_string s) {
  m_container[c].m_slot = s;
  m_container[c].m_useOption = false;
}

void
OptionParser::insert_option(char c, slot_string s) {
  m_container[c].m_slot = s;
  m_container[c].m_useOption = true;
}

void
OptionParser::insert_option_list(char c, slot_string_pair s) {
  m_container[c].m_slot = std::tr1::bind(&OptionParser::call_option_list, s, std::tr1::placeholders::_1);
  m_container[c].m_useOption = true;
}

void
OptionParser::insert_int_pair(char c, slot_int_pair s) {
  m_container[c].m_slot = std::tr1::bind(&OptionParser::call_int_pair, s, std::tr1::placeholders::_1);
  m_container[c].m_useOption = true;
}

int
OptionParser::process(int argc, char** argv) {
  int c;
  std::string optString = create_optstring();

  while ((c = getopt(argc, argv, optString.c_str())) != -1)
    if (c == '?')
      throw std::runtime_error("Invalid/unknown option flag \"-" + std::string(1, (char)optopt) + "\". See rtorrent -h for more information.");
    else
      call(c, optarg ? optarg : "");

  return optind;
}

bool
OptionParser::has_flag(char flag, int argc, char** argv) {
  char options[3] = { '-', flag, '\0' };

  return std::find_if(argv, argv + argc, std::not1(std::bind1st(std::ptr_fun(&std::strcmp), options))) != argv + argc;
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
OptionParser::call_option_list(slot_string_pair slot, const std::string& arg) {
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
OptionParser::call_int_pair(slot_int_pair slot, const std::string& arg) {
  int a, b;

  if (std::sscanf(arg.c_str(), "%u-%u", &a, &b) != 2)
    throw std::runtime_error("Invalid argument, \"" + arg + "\" should be \"a-b\"");

  slot(a, b);
}
